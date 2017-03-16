#include "app.h"

#include <QFileInfo>
#include <QDir>

App::App(int &argc, char **argv) :
	QtBackgroundProcess::App(argc, argv),
	config(nullptr),
	mainPool(nullptr),
	connector(nullptr),
	database(nullptr),
	starter(nullptr),
	lastError(),
	dbRdy(false)
{
	QCoreApplication::setApplicationName(QStringLiteral(TARGET));
	QCoreApplication::setApplicationVersion(QStringLiteral(VERSION));
	QCoreApplication::setOrganizationName(QStringLiteral(COMPANY));
	QCoreApplication::setOrganizationDomain(QStringLiteral("de.skycoder42"));

	connect(this, &App::newTerminalConnected,
			this, &App::terminalConnected);
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
												"by passing one or more of the following modes (in this order):\n"
												"- devices: Will delete all devices that did not connect since <t> days\n"
												"- users: Will delete all users without devices, including their data\n"
												"- data: Will remove remaining data without references\n"
												"- resync: Sets the resync flag for ALL devices/users\n"
												"Note: By passing --accept, you can skip the confirmation"),
								 QStringLiteral("[<commands>|cleanup [devices <t>] [users] [data] [resync]]"));
}

int App::startupApp(const QCommandLineParser &parser)
{
	config = new QSettings(parser.value(QStringLiteral("config-file")), QSettings::IniFormat, this);

	connect(this, &App::aboutToQuit,
			this, &App::quitting,
			Qt::DirectConnection);

	mainPool = new QThreadPool(this);
	auto threadCount = config->value(QStringLiteral("threads"), QThread::idealThreadCount()).toInt();
	mainPool->setMaxThreadCount(threadCount);

	database = new DatabaseController(this);
	connect(database, &DatabaseController::databaseInitDone,
			this, &App::dbDone,
			Qt::QueuedConnection);

	connector = new ClientConnector(database, this);
	if(!connector->setupWss()) {
		lastError = QStringLiteral("Failed to set up secure websocket server. Check error log for details.\n");
		return EXIT_SUCCESS;
	}
	if(!connector->listen()) {
		lastError = QStringLiteral("Failed start websocket server. Check error log for details.\n");
		return EXIT_SUCCESS;
	}

	connect(database, &DatabaseController::notifyChanged,
			connector, &ClientConnector::notifyChanged,
			Qt::QueuedConnection);

	return EXIT_SUCCESS;
}

bool App::requestAppShutdown(QtBackgroundProcess::Terminal *terminal, int &exitCode)
{
	terminal->write("Stopping " + config->value(QStringLiteral("server/name"), QCoreApplication::applicationName()).toByteArray() + " [ ");
	if(starter) {
		terminal->write("\033[1;31mfail\033[0m ]\n"
						"App cannot be stop while starting!\n");
		terminal->flush();
		return false;
	} else {
		terminal->write("\033[1;32mokay\033[0m ]\n");
		terminal->flush();
		return true;
	}
}

void App::terminalConnected(QtBackgroundProcess::Terminal *terminal)
{
	if(terminal->isStarter()) {
		starter = terminal;
		starter->write("Starting " + config->value(QStringLiteral("server/name"), QCoreApplication::applicationName()).toByteArray() + " [ .... ]");
		starter->flush();
		if(dbRdy)
			completeStart();
	} else {
		terminal->disconnectTerminal();
	}
}

void App::quitting()
{
	mainPool->clear();
	mainPool->waitForDone();
}

void App::dbDone(bool ok)
{
	dbRdy = true;
	if(!ok)
		lastError = QStringLiteral("Failed to setup database. Check error log for details.\n");
	if(starter)
		completeStart();
}

void App::completeStart()
{
	if(lastError.isEmpty()) {
		starter->write("\b\b\b\b\b\b\033[1;32mokay\033[0m ]\n");
		starter->disconnectTerminal();
	} else {
		starter->write("\b\b\b\b\b\b\033[1;31mfail\033[0m ]\n");
		starter->write(lastError.toUtf8());
		starter->disconnectTerminal();
		qApp->exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[])
{
	App a(argc, argv);
	return a.exec();
}
