#include "qmldatasync_plugin.h"

#include <QtQml>

#include "qqmldatastore.h"
#include "qqmlsyncmanager.h"

QtDataSyncDeclarativeModule::QtDataSyncDeclarativeModule(QObject *parent) :
	QQmlExtensionPlugin(parent)
{}

void QtDataSyncDeclarativeModule::registerTypes(const char *uri)
{
	Q_ASSERT(qstrcmp(uri, "de.skycoder42.QtDataSync") == 0);

	qmlRegisterType<QtDataSync::QQmlDataStore>(uri, 1, 0, "DataStore");
	qmlRegisterType<QtDataSync::QQmlSyncManager>(uri, 1, 0, "SyncManager");
}
