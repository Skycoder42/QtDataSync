#include "androidkeystoreplugin.h"
#include "androidkeystore.h"

#include <QtDataSync/defaults.h>

AndroidKeyStorePlugin::AndroidKeyStorePlugin(QObject *parent) :
	QObject(parent),
	KeyStorePlugin()
{}

bool AndroidKeyStorePlugin::keystoreAvailable(const QString &provider) const
{
	if(provider == QStringLiteral("android"))
		return true;
	else
		return false;
}

QtDataSync::KeyStore *AndroidKeyStorePlugin::createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent)
{
	if(provider == QStringLiteral("android"))
		return new AndroidKeyStore(defaults, parent);
	else
		return nullptr;
}
