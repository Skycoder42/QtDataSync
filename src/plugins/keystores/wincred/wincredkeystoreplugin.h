#ifndef WINCREDKEYSTOREPLUGIN_H
#define WINCREDKEYSTOREPLUGIN_H

#include <QtDataSync/keystore.h>

class WinCredKeyStorePlugin : public QObject, public QtDataSync::KeyStorePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QtDataSync_KeyStorePlugin_Iid FILE "wincred.json")
	Q_INTERFACES(QtDataSync::KeyStorePlugin)

public:
	WinCredKeyStorePlugin(QObject *parent = nullptr);

	bool keystoreAvailable(const QString &provider) const override;
	QtDataSync::KeyStore *createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent) override;
};

#endif // WINCREDKEYSTOREPLUGIN_H
