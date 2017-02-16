#include "app.h"

#include <QFileInfo>
#include <QDir>

App::App(int &argc, char **argv) :
	QtBackgroundProcess::App(argc, argv),
	config(nullptr),
	mainPool(nullptr),
	connector(nullptr),
	database(nullptr)
{}

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
	auto path = "/etc/QtDataSyncServer/setup.conf";
#elif
	auto path = QCoreApplication::applicationDirPath() + "/setup.conf";
#endif
	parser.addOption({
						 {"c", "config-file"},
						 "The <path> to the configuration file. The default path depends on the platform",
						 "path",
						 path
					 });
}

int App::startupApp(const QCommandLineParser &parser)
{
	config = new QSettings(parser.value("config-file"), QSettings::IniFormat, this);

	mainPool = new QThreadPool(this);
	auto threadCount = config->value("threads", QThread::idealThreadCount()).toInt();
	mainPool->setMaxThreadCount(threadCount);

	database = new DatabaseController(this);
	connector = new ClientConnector(database, this);
	if(!connector->setupWss())
		return EXIT_FAILURE;
	if(!connector->listen())
		return EXIT_FAILURE;

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
