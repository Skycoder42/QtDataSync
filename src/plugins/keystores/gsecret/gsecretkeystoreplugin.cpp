#include "gsecretkeystoreplugin.h"
#include "gsecretkeystore.h"

#include "libsecretwrapper.h"

GSecretKeyStorePlugin::GSecretKeyStorePlugin(QObject *parent) :
	QObject(parent),
	KeyStorePlugin()
{}

QtDataSync::KeyStore *GSecretKeyStorePlugin::createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent)
{
	if(provider == QStringLiteral("gsecret")) //only create if actually enabled
		return new GSecretKeyStore(defaults, parent);
	else
		return nullptr;
}
