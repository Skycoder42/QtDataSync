#include "app.h"

#include <QFileInfo>
#include <QDir>
#include <QTimer>

App::App(int &argc, char **argv) :
	QtBackgroundProcess::App(argc, argv),
	config(nullptr),
	mainPool(nullptr),
	connector(nullptr),
	database(nullptr),
	currentTerminal(nullptr),
	lastError(),
	dbRdy(false)
{
	QCoreApplication::setApplicationName(QStringLiteral(TARGET));
	QCoreApplication::setApplicationVersion(QStringLiteral(VERSION));
	QCoreApplication::setOrganizationName(QStringLiteral(COMPANY));
	QCoreApplication::setOrganizationDomain(QStringLiteral(BUNDLE_PREFIX));

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

	parser.addPositionalArgument(QStringLiteral("cleanup"),
								 QStringLiteral("Perform a cleanup operation on the running server. "
												"Will delete all devices that did not connect since <t> days"),
								 QStringLiteral("[<commands>|cleanup <t>]"));
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
	mainPool->setExpiryTimeout(10 * 60 * 1000); // 10 min

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
		startCleanup();
	} else
		terminal->disconnectTerminal();
}

void App::quitting()
{
	mainPool->clear();
	mainPool->waitForDone();
}

void App::dbDone(bool ok)
{
	dbRdy = true;
	if(!ok) {
		lastError = QStringLiteral("Failed to setup database. Check error log for details.");
		if(!currentTerminal) {
			QTimer::singleShot(5000, this, [](){
				qApp->exit(EXIT_FAILURE);
			});
		}
	}
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
	if(params.isEmpty())
		cleanupOperationDone(0, QStringLiteral("\033[1;33mWarning:\033[0m No minimum offline days specified!"));
	else {
		auto days = params.first().toULongLong();
		if(days == 0)
			cleanupOperationDone(0, QStringLiteral("Invalid day number."));
		else
			database->cleanupDevices(days);
	}
}

void App::cleanupOperationDone(int rowsAffected, const QString &error)
{
	if(error.isEmpty()) {
		currentTerminal->writeLine("Removed devices: " + QByteArray::number(rowsAffected), false);
		currentTerminal->writeLine("\033[1;36mCleanup completed!\033[0m");
	} else {
		currentTerminal->writeLine(error.toUtf8(), false);
		currentTerminal->writeLine("\033[1;31mCleanup failed!\033[0m");
	}
	currentTerminal->disconnectTerminal();
}

int main(int argc, char *argv[])
{
	App a(argc, argv);
	return a.exec();
}
