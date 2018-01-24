#include "qmldatasync_plugin.h"

#include <QtQml>

#include "qqmldatastore.h"
#include "qqmldatastoremodel.h"
#include "qqmlsyncmanager.h"
#include "qqmlaccountmanager.h"
#include "qqmluserexchangemanager.h"

QtDataSyncDeclarativeModule::QtDataSyncDeclarativeModule(QObject *parent) :
	QQmlExtensionPlugin(parent)
{}

void QtDataSyncDeclarativeModule::registerTypes(const char *uri)
{
	Q_ASSERT(qstrcmp(uri, "de.skycoder42.QtDataSync") == 0);

	qmlRegisterUncreatableType<QtDataSync::DeviceInfo>(uri, 1, 0, "DeviceInfo", QStringLiteral("Q_GADGETS cannot be created from QML"));
	qmlRegisterUncreatableType<QtDataSync::LoginRequest>(uri, 1, 0, "LoginRequest", QStringLiteral("Q_GADGETS cannot be created from QML"));
	qmlRegisterUncreatableType<QtDataSync::UserInfo>(uri, 1, 0, "UserInfo", QStringLiteral("Q_GADGETS cannot be created from QML"));

	qmlRegisterType<QtDataSync::QQmlDataStore>(uri, 1, 0, "DataStore");
	qmlRegisterType<QtDataSync::QQmlDataStoreModel>(uri, 1, 0, "DataStoreModel");
	qmlRegisterType<QtDataSync::QQmlSyncManager>(uri, 1, 0, "SyncManager");
	qmlRegisterType<QtDataSync::QQmlAccountManager>(uri, 1, 0, "AccountManager");
	qmlRegisterType<QtDataSync::QQmlUserExchangeManager>(uri, 1, 0, "UserExchangeManager");
}
