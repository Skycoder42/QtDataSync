#ifndef GSECRETKEYSTORE_H
#define GSECRETKEYSTORE_H

#include <QtCore/QScopedPointer>

#include <QtDataSync/keystore.h>

#include "libsecretwrapper.h"

class GSecretKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit GSecretKeyStore(const QtDataSync::Defaults &defaults, QObject *parent = nullptr);

	QString providerName() const override;
	void loadStore() override;
	void closeStore() override;
	bool contains(const QString &key) const override;
	void storePrivateKey(const QString &key, const QByteArray &pKey) override;
	QByteArray loadPrivateKey(const QString &key) override;
	void remove(const QString &key) override;

private:
	QScopedPointer<LibSecretWrapper> _libSecret;
};

#endif // GSECRETKEYSTORE_H
