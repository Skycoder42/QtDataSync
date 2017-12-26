#include "secretservicekeystoreplugin.h"
#include "secretservicekeystore.h"

#include "libsecretwrapper.h"

SecretServiceKeyStorePlugin::SecretServiceKeyStorePlugin(QObject *parent) :
	QObject(parent),
	KeyStorePlugin()
{}

QtDataSync::KeyStore *SecretServiceKeyStorePlugin::createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent)
{
	if(provider == QStringLiteral("secretservice") ||
	   provider == QStringLiteral("gnome-keyring")) //only create if actually enabled
		return new SecretServiceKeyStore(defaults, provider, parent);
	else
		return nullptr;
}
