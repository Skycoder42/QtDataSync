#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtAndroid>
#include <QtDataSyncAndroid/AndroidSyncControl>
#include "syncservice.h"

namespace {

//! [activity_main]
int activityMain(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication app(argc, argv);

	QAndroidJniObject::callStaticMethod<void>("de/skycoder42/qtdatasync/sample/androidsync/SvcHelper",
											  "registerNotificationChannel",
											  "(Landroid/content/Context;)V",
											  QtAndroid::androidContext().object());

	QQmlApplicationEngine engine;
	engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
	if (engine.rootObjects().isEmpty())
		return -1;

	return app.exec();
}
//! [activity_main]

//! [service_main]
int serviceMain(int argc, char *argv[])
{
	SyncService service{argc, argv};
	return service.exec();
}
//! [service_main]

}

//! [actual_main]
int main(int argc, char *argv[])
{
	for(auto i = 0; i < argc; i++) {
		if(qstrcmp(argv[i], "--backend") == 0)
			return serviceMain(argc, argv);
	}
	return activityMain(argc, argv);
}
//! [actual_main]

