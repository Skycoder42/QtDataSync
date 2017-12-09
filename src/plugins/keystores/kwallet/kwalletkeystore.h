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
	bool contains(const QString &key) const override;
	bool storePrivateKey(const QString &key, const QSslKey &pKey) override;
	QSslKey loadPrivateKey(const QString &key) override;
	bool remove(const QString &key) override;

private:
	QPointer<KWallet::Wallet> _wallet;
};

#endif // KWALLETKEYSTORE_H
