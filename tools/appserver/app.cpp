#include "app.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtCore/QStandardPaths>

#include <QCtrlSignals>

#include "message_p.h"

using namespace std::chrono;

App::App(int &argc, char **argv) :
	QCoreApplication(argc, argv),
	_config(nullptr),
	_mainPool(nullptr),
	_connector(nullptr),
	_database(nullptr)
{
	QCoreApplication::setApplicationName(QStringLiteral(APPNAME));
	QCoreApplication::setApplicationVersion(QStringLiteral(VERSION));
	QCoreApplication::setOrganizationName(QStringLiteral(COMPANY));
	QCoreApplication::setOrganizationDomain(QStringLiteral(BUNDLE_PREFIX));

	//setup logging
	qSetMessagePattern(QStringLiteral("[%{time} "
									  "%{if-debug}\033[32mDebug\033[0m]    %{endif}"
									  "%{if-info}\033[36mInfo\033[0m]     %{endif}"
									  "%{if-warning}\033[33mWarning\033[0m]  %{endif}"
									  "%{if-critical}\033[31mCritical\033[0m] %{endif}"
									  "%{if-fatal}\033[35mFatal\033[0m]    %{endif}"
									  "%{if-category}%{category}: %{endif}"
									  "%{message}"));

	QtDataSync::Message::registerTypes();

	connect(this, &App::aboutToQuit,
			this, &App::stop,
			Qt::DirectConnection);

	auto handler = QCtrlSignalHandler::instance();
	handler->setAutoQuitActive(true);
#ifdef Q_OS_UNIX
	connect(handler, &QCtrlSignalHandler::ctrlSignal,
			this, &App::onSignal);
	handler->registerForSignal(SIGHUP);
	handler->registerForSignal(SIGCONT);
	handler->registerForSignal(SIGTSTP);
	handler->registerForSignal(SIGUSR1);
#endif
}

const QSettings *App::configuration() const
{
	return _config;
}

QThreadPool *App::threadPool() const
{
	return _mainPool;
}

QString App::absolutePath(const QString &path) const
{
	auto dir = QFileInfo(_config->fileName()).dir();
	return QDir::cleanPath(dir.absoluteFilePath(path));
}

bool App::start(const QString &serviceName)
{
	Q_UNUSED(serviceName)

	auto configPath = findConfig();
	if(configPath.isEmpty()) {
		qCritical() << "Unable to find any configuration file. Set it explicitly via the QDSAPP_CONFIG_FILE environment variable";
		return false;
	}

	_config = new QSettings(configPath, QSettings::IniFormat, this);
	if(_config->status() != QSettings::NoError) {
		qCritical() << "Failed to read configuration file"
					<< configPath
					<< "with error:"
					<< (_config->status() == QSettings::AccessError ? "Access denied" : "Invalid format");
		return false;
	}

	//before anything else: read log level
	//NOTE move to service lib
	auto logLevel = _config->value(QStringLiteral("loglevel"),
#ifndef QT_NO_DEBUG
								   4
#else
								   3
#endif
								   ).toInt();
	QString logStr;
	switch (logLevel) {
	case 0: //log nothing
		logStr.prepend(QStringLiteral("\n*.critical=false"));
		Q_FALLTHROUGH();
	case 1: //log critical
		logStr.prepend(QStringLiteral("\n*.warning=false"));
		Q_FALLTHROUGH();
	case 2: //log warn
		logStr.prepend(QStringLiteral("\n*.info=false"));
		Q_FALLTHROUGH();
	case 3: //log info
		logStr.prepend(QStringLiteral("*.debug=false"));
		QLoggingCategory::setFilterRules(logStr);
		break;
	case 4: //log any
	default:
		break;
	}

	qDebug() << "Using configuration:" << _config->fileName();

	_mainPool = new QThreadPool(this);
	_mainPool->setMaxThreadCount(_config->value(QStringLiteral("threads/count"),
											  QThread::idealThreadCount()).toInt());
	auto timeoutMin = _config->value(QStringLiteral("threads/expire"), 10).toInt(); //in minutes
	_mainPool->setExpiryTimeout(duration_cast<milliseconds>(minutes(timeoutMin)).count());
	qDebug() << "Running with max" << _mainPool->maxThreadCount()
			 << "threads and an expiry timeout of" << timeoutMin
			 << "minutes";

	_database = new DatabaseController(this);
	_connector = new ClientConnector(_database, this);

	connect(_database, &DatabaseController::databaseInitDone,
			this, &App::completeStartup,
			Qt::QueuedConnection);

	if(!_connector->setupWss())
		return false;
	_database->initialize();

	qDebug() << QCoreApplication::applicationName() << "started successfully";
	return true;
}

void App::stop()
{
	qDebug() << "Stopping server...";
	_connector->disconnectAll();
	_mainPool->clear();
	_mainPool->waitForDone();
	qDebug() << "Server stopped";
}

void App::pause()
{
	qDebug() << "Pausing server...";
	_connector->setPaused(true);
}

void App::resume()
{
	qDebug() << "Resuming server...";
	_connector->setPaused(false);
}

void App::reload()
{
	Q_UNIMPLEMENTED();
}

void App::processCommand(int code)
{
	switch (code) {
	case CleanupCode:
		_database->cleanupDevices();
	default:
		break;
	}
}

void App::completeStartup(bool ok)
{
	if(!ok || !_connector->listen())
		qApp->exit();
}

void App::onSignal(int signal)
{
#ifdef Q_OS_UNIX
	switch (signal) {
	case SIGHUP:
		reload();
		break;
	case SIGCONT:
		resume();
		break;
	case SIGTSTP:
		pause();
		break;
	case SIGUSR1:
		processCommand(CleanupCode);
		break;
	default:
		break;
	}
#endif
}

QString App::findConfig() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	auto configPath = qEnvironmentVariable("QDSAPP_CONFIG_FILE");
#else
	auto configPath = QString::fromUtf8(qgetenv("QDSAPP_CONFIG_FILE"));
#endif
	if(!configPath.isEmpty())
		return configPath;

	//if not set: empty list
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	auto tmpPaths = qEnvironmentVariable("QDSAPP_CONFIG_PATH").split(QDir::listSeparator(), QString::SkipEmptyParts);
#else
	auto tmpPaths = QString::fromUtf8(qgetenv("QDSAPP_CONFIG_PATH")).split(QDir::listSeparator(), QString::SkipEmptyParts);
#endif
	tmpPaths.append(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation));
	tmpPaths.append(QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation));
#ifdef Q_OS_UNIX
	tmpPaths.append(QStringLiteral("/etc/"));
#else
	tmpPaths.append(QCoreApplication::applicationDirPath());
#endif

	QStringList configNames {
		QStringLiteral("%1.conf").arg(QCoreApplication::applicationName()),
		QStringLiteral("%1.conf").arg(QStringLiteral(APPNAME))
	};

	tmpPaths.removeDuplicates();
	configNames.removeDuplicates();
	for(auto path : tmpPaths) {
		QDir dir(path);
		for(auto config : configNames) {
			if(dir.exists(config))
				return dir.absoluteFilePath(config);
		}
	}

	return {};
}

int main(int argc, char *argv[])
{
	App a(argc, argv);
	if(!a.start({}))
		return EXIT_FAILURE;
	return a.exec();
}
