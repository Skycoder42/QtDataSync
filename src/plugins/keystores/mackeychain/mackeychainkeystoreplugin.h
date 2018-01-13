#ifndef MACKEYCHAINKEYSTOREPLUGIN_H
#define MACKEYCHAINKEYSTOREPLUGIN_H

#include <QtDataSync/keystore.h>

class MacKeychainKeyStorePlugin : public QObject, public QtDataSync::KeyStorePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QtDataSync_KeyStorePlugin_Iid FILE "mackeychain.json")
	Q_INTERFACES(QtDataSync::KeyStorePlugin)

public:
	MacKeychainKeyStorePlugin(QObject *parent = nullptr);

	bool keystoreAvailable(const QString &provider) const override;
	QtDataSync::KeyStore *createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent) override;
};

#endif // MACKEYCHAINKEYSTOREPLUGIN_H
