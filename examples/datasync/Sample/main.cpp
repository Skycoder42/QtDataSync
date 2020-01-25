#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QtWebView/QtWebView>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <QtDataSync/Engine>
#include <QtDataSync/Setup>
#include <QtDataSync/GoogleAuthenticator>
using namespace QtDataSync;
using namespace std::chrono_literals;

int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QtWebView::initialize();

	QGuiApplication app(argc, argv);
	QCoreApplication::setApplicationName(QStringLiteral("qtdatasync-example"));
	QCoreApplication::setOrganizationName(QStringLiteral("Skycoder42"));
	QCoreApplication::setOrganizationDomain(QStringLiteral("de.skycoder42"));
	//QGuiApplication::setQuitOnLastWindowClosed(false);

	try {
		QCommandLineParser parser;
		parser.addHelpOption();
		parser.addPositionalArgument(QStringLiteral("firebase-config"),
									 QStringLiteral("The path to the firebase configuration file, i.e the 'google-services.json'."),
									 QStringLiteral("<firebase-config>"));
		parser.process(app);
		if (parser.positionalArguments().size() < 1)
			parser.showHelp(EXIT_FAILURE);

		auto dsEngine = Setup<GoogleAuthenticator>::fromConfig(parser.positionalArguments()[0], ConfigType::WebConfig)
							.enableNtpSync()
							.createEngine(qApp);
		QObject::connect(qApp, &QCoreApplication::aboutToQuit, [dsEngine](){
			if (dsEngine->isRunning()) {
				dsEngine->stop();
				dsEngine->waitForStopped(5s);
			}
		});

		// create database
		const QDir dbDir{QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)};
		dbDir.mkpath(QStringLiteral("."));
		auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
		db.setDatabaseName(dbDir.absoluteFilePath(QStringLiteral("test.db")));
		if (!db.open()) {
			qCritical() << db.lastError();
			return EXIT_FAILURE;
		}
		if (!db.tables().contains(QStringLiteral("Test"))) {
			QSqlQuery tc{db};
			if (!tc.exec(QStringLiteral("CREATE TABLE Test ("
								   "	id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE, "
								   "	name	TEXT NOT NULL, "
								   "	data	BLOB NOT NULL "
										");"))) {
				qCritical() << tc.lastError();
				return EXIT_FAILURE;
			}
		}
		dsEngine->syncTable(QStringLiteral("Test"), false, db);

		QQmlApplicationEngine engine;
		const QUrl url(QStringLiteral("qrc:/main.qml"));
		QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
			&app, [url](QObject *obj, const QUrl &objUrl) {
				if (!obj && url == objUrl)
					QCoreApplication::exit(-1);
			}, Qt::QueuedConnection);
		engine.rootContext()->setContextProperty(QStringLiteral("auth"), QVariant::fromValue(dsEngine->authenticator()));
		engine.load(url);

		dsEngine->start();
		return app.exec();
	} catch (std::exception &e) {
		qCritical() << e.what();
		return EXIT_FAILURE;
	}
}
