#include "qtdatasync_plugin.h"

#include <QtQml>

#ifdef Q_OS_ANDROID
#include <QtDataSyncAndroid/AndroidSyncControl>
#endif
#ifdef Q_OS_IOS
#include <QtDataSyncIos/IosSyncDelegate>
#include "qqmliossyncsingleton.h"
#endif

#include "qqmldatastore.h"
#include "qqmldatastoremodel.h"
#include "qqmlsyncmanager.h"
#include "qqmlaccountmanager.h"
#include "qqmluserexchangemanager.h"
#include "qqmleventcursor.h"

namespace {

QObject *createEventLogInstance(QQmlEngine *qmlEngine, QJSEngine *)
{
	return new QtDataSync::QQmlEventCursor{qmlEngine};
}

#ifdef Q_OS_IOS
QObject *createIosSyncSingletonInstance(QQmlEngine *qmlEngine, QJSEngine *)
{
	return new QtDataSync::QQmlIosSyncSingleton{qmlEngine};
}
#endif

}

QtDataSyncDeclarativeModule::QtDataSyncDeclarativeModule(QObject *parent) :
	QQmlExtensionPlugin(parent)
{}

void QtDataSyncDeclarativeModule::registerTypes(const char *uri)
{
	Q_ASSERT(qstrcmp(uri, "de.skycoder42.QtDataSync") == 0);

	//Version 4.0
	qmlRegisterUncreatableType<QtDataSync::DeviceInfo>(uri, 4, 0, "DeviceInfo", QStringLiteral("Q_GADGETS cannot be created from QML"));
	qmlRegisterUncreatableType<QtDataSync::LoginRequest>(uri, 4, 0, "LoginRequest", QStringLiteral("Q_GADGETS cannot be created from QML"));
	qmlRegisterUncreatableType<QtDataSync::UserInfo>(uri, 4, 0, "UserInfo", QStringLiteral("Q_GADGETS cannot be created from QML"));

	qmlRegisterType<QtDataSync::QQmlDataStore>(uri, 4, 0, "DataStore");
	qmlRegisterType<QtDataSync::QQmlDataStoreModel>(uri, 4, 0, "DataStoreModel");
	qmlRegisterType<QtDataSync::QQmlSyncManager>(uri, 4, 0, "SyncManager");
	qmlRegisterType<QtDataSync::QQmlAccountManager>(uri, 4, 0, "AccountManager");
	qmlRegisterType<QtDataSync::QQmlUserExchangeManager>(uri, 4, 0, "UserExchangeManager");

	//Version 4.1
	qmlRegisterModule(uri, 4, 1);

	//Version 4.2
	qmlRegisterUncreatableType<QtDataSync::EventCursor>(uri, 4, 2, "EventCursor", QStringLiteral("Use the EventLog singleton to create EventCursors"));
	qmlRegisterSingletonType<QtDataSync::QQmlEventCursor>(uri, 4, 2, "EventLog", createEventLogInstance);
#ifdef Q_OS_ANDROID
	qmlRegisterType<QtDataSync::AndroidSyncControl>(uri, 4, 2, "AndroidSyncControl");
#endif
#ifdef Q_OS_IOS
	qmlRegisterUncreatableType<QtDataSync::IosSyncDelegate>(uri, 4, 2, "IosSyncDelegate", QStringLiteral("Use the IosSyncSingleton singleton to obtain the current IosSyncDelegate"));
	qmlRegisterSingletonType<QtDataSync::QQmlIosSyncSingleton>(uri, 4, 2, "IosSyncSingleton", createIosSyncSingletonInstance);
#endif

	//Version 4.3
	qmlRegisterModule(uri, 4, 3);

	// Check to make shure no module update is forgotten
	static_assert(VERSION_MAJOR == 4 && VERSION_MINOR == 3, "QML module version needs to be updated");

}
