#ifndef KEYCHAINKEYSTOREPLUGIN_H
#define KEYCHAINKEYSTOREPLUGIN_H

#include <QtDataSync/keystore.h>

class KeychainKeyStorePlugin : public QObject, public QtDataSync::KeyStorePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QtDataSync_KeyStorePlugin_Iid FILE "keychain.json")
	Q_INTERFACES(QtDataSync::KeyStorePlugin)

public:
	KeychainKeyStorePlugin(QObject *parent = nullptr);

	bool keystoreAvailable(const QString &provider) const override;
	QtDataSync::KeyStore *createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent) override;
};

#endif // KEYCHAINKEYSTOREPLUGIN_H
