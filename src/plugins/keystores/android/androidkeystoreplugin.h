#ifndef ANDROIDKEYSTOREPLUGIN_H
#define ANDROIDKEYSTOREPLUGIN_H

#include <QtDataSync/keystore.h>

class AndroidKeyStorePlugin : public QObject, public QtDataSync::KeyStorePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QtDataSync_KeyStorePlugin_Iid FILE "android.json")
	Q_INTERFACES(QtDataSync::KeyStorePlugin)

public:
	AndroidKeyStorePlugin(QObject *parent = nullptr);

	bool keystoreAvailable(const QString &provider) const override;
	QtDataSync::KeyStore *createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent) override;
};

#endif // ANDROIDKEYSTOREPLUGIN_H
