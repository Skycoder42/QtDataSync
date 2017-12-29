#include "kwalletkeystoreplugin.h"
#include "kwalletkeystore.h"

KWalletKeyStorePlugin::KWalletKeyStorePlugin(QObject *parent) :
	QObject(parent),
	KeyStorePlugin()
{}

QtDataSync::KeyStore *KWalletKeyStorePlugin::createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent)
{
	if(provider == QStringLiteral("kwallet"))
		return new KWalletKeyStore(defaults, parent);
	else
		return nullptr;
}

bool KWalletKeyStorePlugin::keystoreAvailable(const QString &provider) const
{
	if(provider == QStringLiteral("kwallet"))
		return KWallet::Wallet::isEnabled();
	else
		return false;
}
