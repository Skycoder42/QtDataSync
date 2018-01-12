#ifndef WINCREDKEYSTORE_H
#define WINCREDKEYSTORE_H

#include <QtDataSync/keystore.h>

class WinCredKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit WinCredKeyStore(const QtDataSync::Defaults &defaults, QObject *parent = nullptr);

	QString providerName() const override;
	bool isOpen() const override;
	void openStore() override;
	void closeStore() override;
	bool contains(const QString &key) const override;
	void save(const QString &key, const QByteArray &pKey) override;
	QByteArray load(const QString &key) override;
	void remove(const QString &key) override;

private:
	bool _open;

	static QString fullKey(const QString &key);
	static QString formatWinError(ulong error);
	QByteArray tryLoad(const QString &key) const;
};

#endif // WINCREDKEYSTORE_H
