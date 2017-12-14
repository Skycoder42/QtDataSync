#ifndef PLAINKEYSTORE_H
#define PLAINKEYSTORE_H

#include <QtCore/QSettings>
#include <QtCore/QPointer>

#include <QtDataSync/keystore.h>

class PlainKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit PlainKeyStore(const QtDataSync::Defaults &defaults, QObject *parent = nullptr);

	void loadStore() override;
	void closeStore() override;
	bool contains(const QString &key) const override;
	void storePrivateKey(const QString &key, const QSslKey &pKey) override;
	QSslKey loadPrivateKey(const QString &key) override;
	void remove(const QString &key) override;

private:
	QPointer<QSettings> _settings;
};

#endif // PLAINKEYSTORE_H
