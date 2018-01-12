#include "wincredkeystoreplugin.h"
#include "wincredkeystore.h"

#include <QtDataSync/defaults.h>

WinCredKeyStorePlugin::WinCredKeyStorePlugin(QObject *parent) :
	QObject(parent),
	KeyStorePlugin()
{}

bool WinCredKeyStorePlugin::keystoreAvailable(const QString &provider) const
{
	if(provider == QStringLiteral("wincred"))
		return true;
	else
		return false;
}

QtDataSync::KeyStore *WinCredKeyStorePlugin::createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent)
{
	if(provider == QStringLiteral("wincred"))
		return new WinCredKeyStore(defaults, parent);
	else
		return nullptr;
}
