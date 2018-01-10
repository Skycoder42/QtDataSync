#ifndef SECRETSERVICEKEYSTORE_H
#define SECRETSERVICEKEYSTORE_H

#include <QtCore/QScopedPointer>

#include <QtDataSync/keystore.h>

#include "libsecretwrapper.h"

class SecretServiceKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit SecretServiceKeyStore(const QtDataSync::Defaults &defaults,
								   const QString &providerName,
								   QObject *parent = nullptr);

	QString providerName() const override;
	bool isOpen() const override;
	void openStore() override;
	void closeStore() override;
	bool contains(const QString &key) const override;
	void save(const QString &key, const QByteArray &pKey) override;
	QByteArray load(const QString &key) override;
	void remove(const QString &key) override;

private:
	const QString _providerName;
	QScopedPointer<LibSecretWrapper> _libSecret;
};

#endif // SECRETSERVICEKEYSTORE_H
