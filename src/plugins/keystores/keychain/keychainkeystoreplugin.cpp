#include "keychainkeystoreplugin.h"
#include "keychainkeystore.h"

#include <QtDataSync/defaults.h>

KeychainKeyStorePlugin::KeychainKeyStorePlugin(QObject *parent) :
	QObject(parent),
	KeyStorePlugin()
{}

bool KeychainKeyStorePlugin::keystoreAvailable(const QString &provider) const
{
	if(provider == QStringLiteral("keychain"))
		return true;
	else
		return false;
}

QtDataSync::KeyStore *KeychainKeyStorePlugin::createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent)
{
	if(provider == QStringLiteral("keychain"))
		return new KeychainKeyStore(defaults, parent);
	else
		return nullptr;
}
