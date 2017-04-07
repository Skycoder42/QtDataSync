#include "app.h"

#include <QFileInfo>
#include <QDir>

App::App(int &argc, char **argv) :
	QtBackgroundProcess::App(argc, argv),
	config(nullptr),
	mainPool(nullptr),
	connector(nullptr),
	database(nullptr),
	currentTerminal(nullptr),
	lastError(),
	dbRdy(false),
	cleanupStage(None)
{
	QCoreApplication::setApplicationName(QStringLiteral(TARGET));
	QCoreApplication::setApplicationVersion(QStringLiteral(VERSION));
	QCoreApplication::setOrganizationName(QStringLiteral(COMPANY));
	QCoreApplication::setOrganizationDomain(QStringLiteral(BUNDLE_PREFIX));

	connect(this, &App::newTerminalConnected,
			this, &App::terminalConnected);
}

void App::loadId()
{
	QCommandLineParser parser;
	setupParser(parser, true);
	if(parser.parse(arguments()) && parser.isSet(QStringLiteral("id")))
		setInstanceID(parser.value(QStringLiteral("id")));
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
	auto path = QStringLiteral("/etc/%1/setup.conf").arg(QStringLiteral(TARGET));
#else
	auto path = QCoreApplication::applicationDirPath() + QStringLiteral("/setup.conf");
#endif
	parser.addOption({
						 {QStringLiteral("c"), QStringLiteral("config-file")},
						 QStringLiteral("The <path> to the configuration file. The default path depends on the platform"),
						 QStringLiteral("path"),
						 path
					 });
	parser.addOption({
						 QStringLiteral("id"),
						 QStringLiteral("A custom <id> to run more than just one instance of the server. NOTE: all terminals "
										"that want to connect to a master must specify the same id"),
						 QStringLiteral("id")
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
	setApplicationName(config->value(QStringLiteral("name"), applicationName()).toString());

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
	connect(database, &DatabaseController::cleanupOperationDone,
			this, &App::cleanupOperationDone,
			Qt::QueuedConnection);

	connector = new ClientConnector(database, this);
	if(!connector->setupWss()) {
		lastError = QStringLiteral("Failed to set up secure websocket server. Check error log for details.");
		return EXIT_SUCCESS;
	}
	if(!connector->listen()) {
		lastError = QStringLiteral("Failed start websocket server. Check error log for details.");
		return EXIT_SUCCESS;
	}

	connect(database, &DatabaseController::notifyChanged,
			connector, &ClientConnector::notifyChanged,
			Qt::QueuedConnection);

	return EXIT_SUCCESS;
}

bool App::requestAppShutdown(QtBackgroundProcess::Terminal *terminal, int &)
{
	terminal->write("Stopping " + QCoreApplication::applicationName().toUtf8() + " [ ");
	if(currentTerminal) {
		terminal->writeLine("\033[1;31mfail\033[0m ]", false);
		terminal->writeLine("App cannot be stop while starting!");
		return false;
	} else {
		terminal->writeLine("\033[1;32mokay\033[0m ]");
		return true;
	}
}

void App::terminalConnected(QtBackgroundProcess::Terminal *terminal)
{
	if(terminal->isStarter()) {
		currentTerminal = terminal;
		currentTerminal->write("Starting " + QCoreApplication::applicationName().toUtf8() + " [ .... ]");
		currentTerminal->flush();
		if(dbRdy)
			completeStart();
	} else if(terminal->parser()->positionalArguments().startsWith(QStringLiteral("cleanup"))) {
		terminal->writeLine("Starting cleanup rountines...");
		if(currentTerminal) {
			terminal->writeLine("\033[1;31mError:\033[0m Another terminal is already performing a major operation!");
			terminal->disconnectTerminal();
			return;
		}
		currentTerminal = terminal;
		cleanupStage = Devices;
		startCleanup();
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
		lastError = QStringLiteral("Failed to setup database. Check error log for details.");
	if(currentTerminal)
		completeStart();
}

void App::completeStart()
{
	if(lastError.isEmpty()) {
		currentTerminal->writeLine("\b\b\b\b\b\b\033[1;32mokay\033[0m ]");
		currentTerminal->disconnectTerminal();
	} else {
		currentTerminal->writeLine("\b\b\b\b\b\b\033[1;31mfail\033[0m ]", false);
		currentTerminal->writeLine(lastError.toUtf8());
		currentTerminal->disconnectTerminal();
		qApp->exit(EXIT_FAILURE);
	}
}

void App::startCleanup()
{
	auto params = currentTerminal->parser()->positionalArguments();
	params.removeFirst();
	if(params.isEmpty()) {
		currentTerminal->writeLine("\033[1;33mWarning:\033[0m No cleanup targets specified!");
		completeCleanup();
	} else {
		//devices
		if(cleanupStage == Devices) {
			auto devicesIndex = params.indexOf(QStringLiteral("devices"));
			if(devicesIndex != -1) {
				lastError = QStringLiteral("After the \"devices\" target the minimum offline days must follow! A number greater than 0 is required.");

				if(++devicesIndex >= params.size()) {
					completeCleanup();
					return;
				}
				auto days = params.value(devicesIndex).toULongLong();
				if(days == 0) {
					completeCleanup();
					return;
				}
				lastError.clear();

				database->cleanupDevices(days);
			} else
				cleanupStage = Users;
		}

		//users
		if(cleanupStage == Users) {
			if(params.contains(QStringLiteral("users")))
				database->cleanupUsers();
			else
				cleanupStage = Data;
		}

		//data
		if(cleanupStage == Data) {
			if(params.contains(QStringLiteral("data")))
				database->cleanupData();
			else
				cleanupStage = Resync;
		}

		//resyc
		if(cleanupStage == Resync) {
			if(params.contains(QStringLiteral("resync")))
				database->cleanupResync();
			else
				completeCleanup();
		}
	}
}

void App::cleanupOperationDone(int rowsAffected, const QString &error)
{
	if(error.isEmpty()) {
		switch (cleanupStage) {
		case None:
			break;
		case Devices:
			currentTerminal->write("Removed devices: ");
			cleanupStage = Users;
			break;
		case Users:
			currentTerminal->write("Removed users: ");
			cleanupStage = Data;
			break;
		case Data:
			currentTerminal->write("Removed data zombies: ");
			cleanupStage = Resync;
			break;
		case Resync:
			currentTerminal->writeLine("Resync flags set!");
			cleanupStage = None;
			break;
		default:
			Q_UNREACHABLE();
			break;
		}

		if(cleanupStage != None) {
			currentTerminal->writeLine(QByteArray::number(rowsAffected));
			startCleanup();
		} else
			completeCleanup();
	} else {
		lastError = error;
		completeCleanup();
	}
}

void App::completeCleanup()
{
	cleanupStage = None;//just to be shure
	if(lastError.isEmpty()) {
		currentTerminal->writeLine("\033[1;36mCleanup completed!\033[0m");
		currentTerminal->disconnectTerminal();
	} else {
		currentTerminal->writeLine(lastError.toUtf8());
		currentTerminal->writeLine("\033[1;31mCleanup failed!\033[0m");
		currentTerminal->disconnectTerminal();
	}
}

int main(int argc, char *argv[])
{
	App a(argc, argv);
	a.loadId();
	return a.exec();
}
