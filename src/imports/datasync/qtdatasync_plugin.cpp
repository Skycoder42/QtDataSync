#include "qtdatasync_plugin.h"

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

	//Version 4.0
	qmlRegisterUncreatableType<QtDataSync::DeviceInfo>(uri, 4, 0, "DeviceInfo", QStringLiteral("Q_GADGETS cannot be created from QML"));
	qmlRegisterUncreatableType<QtDataSync::LoginRequest>(uri, 4, 0, "LoginRequest", QStringLiteral("Q_GADGETS cannot be created from QML"));
	qmlRegisterUncreatableType<QtDataSync::UserInfo>(uri, 4, 0, "UserInfo", QStringLiteral("Q_GADGETS cannot be created from QML"));

	qmlRegisterType<QtDataSync::QQmlDataStore>(uri, 4, 0, "DataStore");
	qmlRegisterType<QtDataSync::QQmlDataStoreModel>(uri, 4, 0, "DataStoreModel");
	qmlRegisterType<QtDataSync::QQmlSyncManager>(uri, 4, 0, "SyncManager");
	qmlRegisterType<QtDataSync::QQmlAccountManager>(uri, 4, 0, "AccountManager");
	qmlRegisterType<QtDataSync::QQmlUserExchangeManager>(uri, 4, 0, "UserExchangeManager");

	// Check to make shure no module update is forgotten
	static_assert(VERSION_MAJOR == 4 && VERSION_MINOR == 0, "QML module version needs to be updated");

}
