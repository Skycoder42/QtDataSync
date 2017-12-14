#include "plainkeystoreplugin.h"
#include "plainkeystore.h"

PlainKeyStorePlugin::PlainKeyStorePlugin(QObject *parent) :
	QObject(parent),
	KeyStorePlugin()
{}

QtDataSync::KeyStore *PlainKeyStorePlugin::createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent)
{
	if(provider == QStringLiteral("plain"))
		return new PlainKeyStore(defaults, parent);
	else
		return nullptr;
}
