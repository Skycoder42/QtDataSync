#include "kwalletkeystoreplugin.h"
#include "kwalletkeystore.h"

KWalletKeyStorePlugin::KWalletKeyStorePlugin(QObject *parent) :
	QObject(parent),
	KeyStorePlugin()
{}

QtDataSync::KeyStore *KWalletKeyStorePlugin::createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent)
{
	if(provider == QStringLiteral("kwallet") && KWallet::Wallet::isEnabled()) //only create if actually enabled
		return new KWalletKeyStore(defaults, parent);
	else
		return nullptr;
}
