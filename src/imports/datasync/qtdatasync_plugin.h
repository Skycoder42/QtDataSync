#ifndef QTDATASYNC_PLUGIN_H
#define QTDATASYNC_PLUGIN_H

#include <QQmlExtensionPlugin>

class QtDataSyncDeclarativeModule : public QQmlExtensionPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
	QtDataSyncDeclarativeModule(QObject *parent = nullptr);
	void registerTypes(const char *uri) override;
};

#endif // QTDATASYNC_PLUGIN_H
