#ifndef PLAINKEYSTOREPLUGIN_H
#define PLAINKEYSTOREPLUGIN_H

#include <QtDataSync/keystore.h>

class PlainKeyStorePlugin : public QObject, public QtDataSync::KeyStorePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QtDataSync_KeyStorePlugin_Iid FILE "plain.json")
	Q_INTERFACES(QtDataSync::KeyStorePlugin)

public:
	PlainKeyStorePlugin(QObject *parent = nullptr);

	QtDataSync::KeyStore *createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent) override;
};

#endif // PLAINKEYSTOREPLUGIN_H
