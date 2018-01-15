#include "app.h"

#include <QFileInfo>
#include <QDir>
#include <QTimer>

#include "message_p.h"

App::App(int &argc, char **argv) :
	QCoreApplication(argc, argv),
	config(nullptr),
	mainPool(nullptr),
	connector(nullptr),
	database(nullptr)
{
	QCoreApplication::setApplicationName(QStringLiteral(TARGET));
	QCoreApplication::setApplicationVersion(QStringLiteral(VERSION));
	QCoreApplication::setOrganizationName(QStringLiteral(COMPANY));
	QCoreApplication::setOrganizationDomain(QStringLiteral(BUNDLE_PREFIX));

	QtDataSync::Message::registerTypes();

	connect(this, &App::aboutToQuit,
			this, &App::stop,
			Qt::DirectConnection);
}

QSettings *App::configuration() const
{
	return config;
}

QThreadPool *App::threadPool() const
{
	return mainPool;
}

QString App::absolutePath(const QString &path) const
{
	auto dir = QFileInfo(config->fileName()).dir();
	return QDir::cleanPath(dir.absoluteFilePath(path));
}

bool App::start(const QString &serviceName)
{
	Q_UNUSED(serviceName)

#ifdef Q_OS_UNIX
	auto configPath = QStringLiteral("/etc/%1/setup.conf").arg(QStringLiteral(TARGET));//TODO more paths?
#else
	auto configPath = QCoreApplication::applicationDirPath() + QStringLiteral("/setup.conf");
#endif
	configPath = qEnvironmentVariable("QDSAPP_CONFIG", configPath);//NOTE document
	config = new QSettings(configPath, QSettings::IniFormat, this);
	if(config->status() != QSettings::NoError) {
		qCritical() << "Failed to read configuration file"
					<< configPath
					<< "with error:"
					<< (config->status() == QSettings::AccessError ? "Access denied" : "Invalid format");
		return false;
	} else
		qDebug() << "Using configuration:" << config->fileName();

	mainPool = new QThreadPool(this);
	mainPool->setMaxThreadCount(config->value(QStringLiteral("threads/count"),
											  QThread::idealThreadCount()).toInt());
	auto timeoutMin = config->value(QStringLiteral("threads/expire"), 10).toInt(); //in minutes
	mainPool->setExpiryTimeout(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(timeoutMin)).count());
	qDebug() << "Running with max" << mainPool->maxThreadCount()
			 << "threads and an expiry timeout of" << timeoutMin
			 << "minutes";

	database = new DatabaseController(this);
	connector = new ClientConnector(database, this);

	connect(database, &DatabaseController::databaseInitDone,
			this, &App::completeStartup,
			Qt::QueuedConnection);

	if(!connector->setupWss())
		return false;
	database->initialize();

	qDebug() << QCoreApplication::applicationName() << "started successfully";
	return true;
}

void App::stop()
{
	connector->disconnectAll();
	mainPool->clear();
	mainPool->waitForDone();
}

void App::pause()
{
	connector->setPaused(true);
}

void App::resume()
{
	connector->setPaused(false);
}

void App::processCommand(int code)
{
	switch (code) {
	case CleanupCode:
		database->cleanupDevices();
	default:
		break;
	}
}

void App::completeStartup(bool ok)
{
	if(!ok || !connector->listen())
		qApp->exit();
}

int main(int argc, char *argv[])
{
	App a(argc, argv);
	QMetaObject::invokeMethod(&a, "start", Qt::QueuedConnection,
							  Q_ARG(QString, QString()));
	return a.exec();
}
