#ifndef PLAINKEYSTORE_H
#define PLAINKEYSTORE_H

#include <QtCore/QSettings>
#include <QtCore/QPointer>

#include <QtDataSync/keystore.h>

class PlainKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit PlainKeyStore(QObject *parent = nullptr);

	bool loadStore() override;
	void closeStore() override;
	bool contains(const QString &key) const override;
	bool storePrivateKey(const QString &key, const QSslKey &pKey) override;
	QSslKey loadPrivateKey(const QString &key) override;
	bool remove(const QString &key) override;

private:
	QPointer<QSettings> _settings;
};

#endif // PLAINKEYSTORE_H
