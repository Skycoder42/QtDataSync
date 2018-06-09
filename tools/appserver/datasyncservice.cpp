#include "datasyncservice.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtCore/QStandardPaths>

#include "message_p.h"

using namespace std::chrono;

DatasyncService::DatasyncService(int &argc, char **argv) :
	Service{argc, argv},
	_config(nullptr),
	_mainPool(nullptr),
	_connector(nullptr),
	_database(nullptr)
{
	QCoreApplication::setApplicationName(QStringLiteral(APPNAME));
	QCoreApplication::setApplicationVersion(QStringLiteral(VERSION));
	QCoreApplication::setOrganizationName(QStringLiteral(COMPANY));
	QCoreApplication::setOrganizationDomain(QStringLiteral(BUNDLE_PREFIX));

	addCallback("SIGUSR1", &DatasyncService::sigusr1);
	addCallback("command", &DatasyncService::command);
}

const QSettings *DatasyncService::configuration() const
{
	return _config;
}

QThreadPool *DatasyncService::threadPool() const
{
	return _mainPool;
}

QString DatasyncService::absolutePath(const QString &path) const
{
	auto dir = QFileInfo(_config->fileName()).dir();
	return QDir::cleanPath(dir.absoluteFilePath(path));
}

QtService::Service::CommandMode DatasyncService::onStart()
{
	QtDataSync::Message::registerTypes();

	auto configPath = findConfig();
	if(configPath.isEmpty()) {
		qCritical() << "Unable to find any configuration file. Set it explicitly via the QDSAPP_CONFIG_FILE environment variable";
		qApp->exit(EXIT_FAILURE);
		return Synchronous;
	}

	_config = new QSettings(configPath, QSettings::IniFormat, this);
	if(_config->status() != QSettings::NoError) {
		qCritical() << "Failed to read configuration file"
					<< configPath
					<< "with error:"
					<< (_config->status() == QSettings::AccessError ? "Access denied" : "Invalid format");
		qApp->exit(EXIT_FAILURE);
		return Synchronous;
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
	_mainPool->setExpiryTimeout(static_cast<int>(duration_cast<milliseconds>(minutes(timeoutMin)).count()));
	qDebug() << "Running with max" << _mainPool->maxThreadCount()
			 << "threads and an expiry timeout of" << timeoutMin
			 << "minutes";

	_database = new DatabaseController(this);
	_connector = new ClientConnector(_database, this);

	connect(_database, &DatabaseController::databaseInitDone,
			this, &DatasyncService::completeStartup,
			Qt::QueuedConnection);

	if(!_connector->setupWss()) {
		qApp->exit(EXIT_FAILURE);
		return Synchronous;
	}
	_database->initialize();

	qDebug() << QCoreApplication::applicationName() << "started successfully";
	return Asynchronous;
}

QtService::Service::CommandMode DatasyncService::onStop(int &exitCode)
{
	qDebug() << "Stopping server...";
	_connector->disconnectAll();
	_mainPool->clear();
	_mainPool->waitForDone();
	exitCode = EXIT_SUCCESS;
	qDebug() << "Server stopped";
	return Synchronous;
}

QtService::Service::CommandMode DatasyncService::onReload()
{
	//TODO implement
	Q_UNIMPLEMENTED();
	return Synchronous;
}

QtService::Service::CommandMode DatasyncService::onPause()
{
	qDebug() << "Pausing server...";
	_connector->setPaused(true);
	return Synchronous;
}

QtService::Service::CommandMode DatasyncService::onResume()
{
	qDebug() << "Resuming server...";
	_connector->setPaused(false);
	return Synchronous;
}

void DatasyncService::completeStartup(bool ok)
{
	if(!ok || !_connector->listen())
		qApp->exit();
	else
		emit started();
}

QString DatasyncService::findConfig() const
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
	for(const auto &path : qAsConst(tmpPaths)) {
		QDir dir(path);
		for(const auto &config : qAsConst(configNames)) {
			if(dir.exists(config))
				return dir.absoluteFilePath(config);
		}
	}

	return {};
}

void DatasyncService::sigusr1()
{
	_database->cleanupDevices();
}

void DatasyncService::command(int cmd)
{
	switch (cmd) {
	case CleanupCode:
		_database->cleanupDevices();
		break;
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
	DatasyncService a(argc, argv);
	return a.exec();
}
