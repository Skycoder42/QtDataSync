#ifndef KWALLETKEYSTORE_H
#define KWALLETKEYSTORE_H

#include <QtCore/QPointer>

#include <QtDataSync/keystore.h>

#include <KWallet>

class KWalletKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit KWalletKeyStore(const QtDataSync::Defaults &defaults, QObject *parent = nullptr);

	void loadStore() override;
	void closeStore() override;
	bool contains(const QString &key) const override;
	void storePrivateKey(const QString &key, const QSslKey &pKey) override;
	QSslKey loadPrivateKey(const QString &key) override;
	void remove(const QString &key) override;

private:
	QPointer<KWallet::Wallet> _wallet;
};

#endif // KWALLETKEYSTORE_H
