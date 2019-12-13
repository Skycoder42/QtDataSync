#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QDebug>
#include <QtWebView/QtWebView>

#include <QtDataSync/Engine>
#include <QtDataSync/Setup>
#include <QtDataSync/IAuthenticator>

int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QtWebView::initialize();

	QGuiApplication app(argc, argv);
	QCoreApplication::setApplicationName(QStringLiteral("qtdatasync-example"));
	QCoreApplication::setOrganizationName(QStringLiteral("Skycoder42"));
	QCoreApplication::setOrganizationDomain(QStringLiteral("de.skycoder42"));

	try {
		QCommandLineParser parser;
		parser.addHelpOption();
		parser.addPositionalArgument(QStringLiteral("firebase-config"),
									 QStringLiteral("The path to the firebase configuration file, i.e the 'google-services.json'."),
									 QStringLiteral("<firebase-config>"));
		parser.process(app);
		if (parser.positionalArguments().size() < 1)
			parser.showHelp(EXIT_FAILURE);

		auto dsEngine = QtDataSync::Setup::fromConfig(parser.positionalArguments()[0])
							.createEngine(qApp);

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
