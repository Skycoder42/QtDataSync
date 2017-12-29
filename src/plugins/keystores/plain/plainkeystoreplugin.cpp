#include "plainkeystoreplugin.h"
#include "plainkeystore.h"

#include <QtDataSync/defaults.h>

PlainKeyStorePlugin::PlainKeyStorePlugin(QObject *parent) :
	QObject(parent),
	KeyStorePlugin()
{}

bool PlainKeyStorePlugin::keystoreAvailable(const QString &provider) const
{
	if(provider == QStringLiteral("plain"))
		return true;
	else
		return false;
}

QtDataSync::KeyStore *PlainKeyStorePlugin::createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent)
{
	if(provider == QStringLiteral("plain"))
		return new PlainKeyStore(defaults, parent);
	else
		return nullptr;
}
