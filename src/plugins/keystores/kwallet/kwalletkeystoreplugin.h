#ifndef KWALLETKEYSTOREPLUGIN_H
#define KWALLETKEYSTOREPLUGIN_H

#include <QtDataSync/keystore.h>

class KWalletKeyStorePlugin : public QObject, public QtDataSync::KeyStorePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QtDataSync_KeyStorePlugin_Iid FILE "kwallet.json")
	Q_INTERFACES(QtDataSync::KeyStorePlugin)

public:
	KWalletKeyStorePlugin(QObject *parent = nullptr);

	bool keystoreAvailable(const QString &provider) const override;
	QtDataSync::KeyStore *createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent) override;
};

#endif // KWALLETKEYSTOREPLUGIN_H
