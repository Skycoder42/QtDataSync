#include "app.h"

#include <QFileInfo>
#include <QDir>

App::App(int &argc, char **argv) :
	QtBackgroundProcess::App(argc, argv),
	config(nullptr),
	mainPool(nullptr),
	connector(nullptr),
	database(nullptr)
{
	QCoreApplication::setApplicationName(QStringLiteral(TARGET));
	QCoreApplication::setApplicationVersion(QStringLiteral(VERSION));
	QCoreApplication::setOrganizationName(QStringLiteral(COMPANY));
	QCoreApplication::setOrganizationDomain(QStringLiteral("de.skycoder42"));
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

void App::setupParser(QCommandLineParser &parser, bool useShortOptions)
{
	QtBackgroundProcess::App::setupParser(parser, useShortOptions);

#ifdef Q_OS_UNIX
	auto path = QStringLiteral("/etc/QtDataSyncServer/setup.conf");
#elif
	auto path = QCoreApplication::applicationDirPath() + QStringLiteral("/setup.conf");
#endif
	parser.addOption({
						 {QStringLiteral("c"), QStringLiteral("config-file")},
						 QStringLiteral("The <path> to the configuration file. The default path depends on the platform"),
						 QStringLiteral("path"),
						 path
					 });

	parser.addPositionalArgument(QStringLiteral("cleanup"),
								 QStringLiteral("Perform a cleanup operation on the running server. Specify what to clean "
												"by passing one or more of the following modes:\n"
												"- users: Will delete all users without devices, including their data\n"
												"- devices: Will delete all devices that did not connect since <t> days\n"
												"- data: Will remove remaining data without references\n"
												"- resync: Sets the resync flag for ALL devices/users\n"
												"Note: By passing --accept, you can skip the confirmation"),
								 QStringLiteral("[<commands>|cleanup [users] [devices <t>] [data] [resync]]"));
}

int App::startupApp(const QCommandLineParser &parser)
{
	config = new QSettings(parser.value(QStringLiteral("config-file")), QSettings::IniFormat, this);

	mainPool = new QThreadPool(this);
	auto threadCount = config->value(QStringLiteral("threads"), QThread::idealThreadCount()).toInt();
	mainPool->setMaxThreadCount(threadCount);

	database = new DatabaseController(this);
	connector = new ClientConnector(database, this);
	if(!connector->setupWss())
		return EXIT_FAILURE;
	if(!connector->listen())
		return EXIT_FAILURE;

	connect(database, &DatabaseController::notifyChanged,
			connector, &ClientConnector::notifyChanged,
			Qt::QueuedConnection);

	connect(this, &App::aboutToQuit,
			this, &App::quitting,
			Qt::DirectConnection);

	return EXIT_SUCCESS;
}

void App::quitting()
{
	mainPool->clear();
	mainPool->waitForDone();
}

int main(int argc, char *argv[])
{
	App a(argc, argv);
	return a.exec();
}
