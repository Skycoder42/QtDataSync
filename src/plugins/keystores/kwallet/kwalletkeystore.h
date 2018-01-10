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

	QString providerName() const override;
	bool isOpen() const override;
	void openStore() override;
	void closeStore() override;
	bool contains(const QString &key) const override;
	void save(const QString &key, const QByteArray &pKey) override;
	QByteArray load(const QString &key) override;
	void remove(const QString &key) override;

private:
	QPointer<KWallet::Wallet> _wallet;
};

#endif // KWALLETKEYSTORE_H
