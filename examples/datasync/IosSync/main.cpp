#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "syncdelegate.h"

//! [delegate_main]
int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication app(argc, argv);

	QtDataSync::IosSyncDelegate::init(new SyncDelegate{&app});
	QtDataSync::IosSyncDelegate::currentDelegate()->setEnabled(true);

	// ...
//! [delegate_main]
	QQmlApplicationEngine engine;
	engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
	if (engine.rootObjects().isEmpty())
		return -1;

	return app.exec();
}
