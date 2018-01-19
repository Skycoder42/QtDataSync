#include "qmldatasync_plugin.h"

#include <QtQml>

#include "qqmldatastore.h"

QtDataSyncDeclarativeModule::QtDataSyncDeclarativeModule(QObject *parent) :
	QQmlExtensionPlugin(parent)
{}

void QtDataSyncDeclarativeModule::registerTypes(const char *uri)
{
	Q_ASSERT(uri == QLatin1String("de.skycoder42.QtDataSync"));

	qmlRegisterType<QtDataSync::QQmlDataStore>(uri, 1, 0, "DataStore");
}
