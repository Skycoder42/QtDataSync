#ifndef GSECRETKEYSTOREPLUGIN_H
#define GSECRETKEYSTOREPLUGIN_H

#include <QtDataSync/keystore.h>

class GSecretKeyStorePlugin : public QObject, public QtDataSync::KeyStorePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QtDataSync_KeyStorePlugin_Iid FILE "gsecret.json")
	Q_INTERFACES(QtDataSync::KeyStorePlugin)

public:
	GSecretKeyStorePlugin(QObject *parent = nullptr);

	QtDataSync::KeyStore *createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent) override;
};

#endif // GSECRETKEYSTOREPLUGIN_H
