#ifndef QMLDATASYNC_PLUGIN_H
#define QMLDATASYNC_PLUGIN_H

#include <QQmlExtensionPlugin>

class QtDataSyncDeclarativeModule : public QQmlExtensionPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
	QtDataSyncDeclarativeModule(QObject *parent = nullptr);
	void registerTypes(const char *uri) override;
};

#endif // QMLDATASYNC_PLUGIN_H
