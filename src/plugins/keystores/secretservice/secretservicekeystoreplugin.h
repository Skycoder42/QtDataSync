#ifndef SECRETSERVICEKEYSTOREPLUGIN_H
#define SECRETSERVICEKEYSTOREPLUGIN_H

#include <QtDataSync/keystore.h>

//TODO test
class SecretServiceKeyStorePlugin : public QObject, public QtDataSync::KeyStorePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QtDataSync_KeyStorePlugin_Iid FILE "secretservice.json")
	Q_INTERFACES(QtDataSync::KeyStorePlugin)

public:
	SecretServiceKeyStorePlugin(QObject *parent = nullptr);

	bool keystoreAvailable(const QString &provider) const override;
	QtDataSync::KeyStore *createInstance(const QString &provider, const QtDataSync::Defaults &defaults, QObject *parent) override;
};

#endif // SECRETSERVICEKEYSTOREPLUGIN_H
