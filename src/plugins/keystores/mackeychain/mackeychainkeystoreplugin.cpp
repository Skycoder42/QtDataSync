#include "mackeychainkeystoreplugin.h"
#include "mackeychainkeystore.h"

#include <QtDataSync/defaults.h>

MacKeychainKeyStorePlugin::MacKeychainKeyStorePlugin(QObject *parent) :
	QObject(parent),
	KeyStorePlugin()
{}

bool MacKeychainKeyStorePlugin::keystoreAvailable(const QString &provider) const
{
	if(provider == QStringLiteral("mackeychain"))
		return true;
	else
		return false;
}

QtDataSync::KeyStore *MacKeychainKeyStorePlugin::createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent)
{
	if(provider == QStringLiteral("mackeychain"))
		return new MacKeychainKeyStore(defaults, parent);
	else
		return nullptr;
}
