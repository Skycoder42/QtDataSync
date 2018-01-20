#include "qmldatasync_plugin.h"

#include <QtQml>

#include "qqmldatastore.h"
#include "qqmlsyncmanager.h"
#include "qqmlaccountmanager.h"

QtDataSyncDeclarativeModule::QtDataSyncDeclarativeModule(QObject *parent) :
	QQmlExtensionPlugin(parent)
{}

void QtDataSyncDeclarativeModule::registerTypes(const char *uri)
{
	Q_ASSERT(qstrcmp(uri, "de.skycoder42.QtDataSync") == 0);

	qmlRegisterType<QtDataSync::QQmlDataStore>(uri, 1, 0, "DataStore");
	qmlRegisterType<QtDataSync::QQmlSyncManager>(uri, 1, 0, "SyncManager");
	qmlRegisterType<QtDataSync::QQmlAccountManager>(uri, 1, 0, "AccountManager");
	qmlRegisterUncreatableType<QtDataSync::DeviceInfo>(uri, 1, 0, "DeviceInfo", tr("Q_GADGETS cannot be created from QML"));
	qmlRegisterUncreatableType<QtDataSync::LoginRequest>(uri, 1, 0, "LoginRequest", tr("Q_GADGETS cannot be created from QML"));
}
