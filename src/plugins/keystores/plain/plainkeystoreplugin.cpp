#include "plainkeystoreplugin.h"
#include "plainkeystore.h"

PlainKeyStorePlugin::PlainKeyStorePlugin(QObject *parent) :
	QObject(parent),
	KeyStorePlugin()
{}

QtDataSync::KeyStore *PlainKeyStorePlugin::createInstance(const QString &provider, QObject *parent)
{
	if(provider == QStringLiteral("plain"))
		return new PlainKeyStore(parent);
	else
		return nullptr;
}
