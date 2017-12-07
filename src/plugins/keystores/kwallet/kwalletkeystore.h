#ifndef KWALLETKEYSTORE_H
#define KWALLETKEYSTORE_H

#include <QtCore/QPointer>

#include <QtDataSync/keystore.h>

#include <KWallet>

class KWalletKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit KWalletKeyStore(QObject *parent = nullptr);

	bool loadStore() override;
	void closeStore() override;
	bool containsSecret(const QString &key) const override;
	bool storeSecret(const QString &key, const QByteArray &secret) override;
	QByteArray loadSecret(const QString &key) override;
	bool removeSecret(const QString &key) override;

private:
	QPointer<KWallet::Wallet> _wallet;
};

#endif // KWALLETKEYSTORE_H
