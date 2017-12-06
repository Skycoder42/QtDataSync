#ifndef PLAINKEYSTORE_H
#define PLAINKEYSTORE_H

#include <QtCore/QSettings>

#include <QtDataSync/keystore.h>

class PlainKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit PlainKeyStore(QObject *parent = nullptr);

	bool canCheckContains() const override;
	bool contains(const QByteArray &key) const override;

public Q_SLOTS:
	void storeSecret(const QByteArray &key, const QByteArray &secret) override;
	void loadSecret(const QByteArray &key) override;

private:
	QSettings *_settings;
};

#endif // PLAINKEYSTORE_H
