#ifndef PLAINKEYSTORE_H
#define PLAINKEYSTORE_H

#include <QtCore/QSettings>

#include <QtDataSync/keystore.h>

class PlainKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit PlainKeyStore(const QtDataSync::Defaults &defaults, QObject *parent = nullptr);

	QString providerName() const override;
	bool isOpen() const override;
	void openStore() override;
	void closeStore() override;
	bool contains(const QString &key) const override;
	void save(const QString &key, const QByteArray &pKey) override;
	QByteArray load(const QString &key) override;
	void remove(const QString &key) override;

private:
	QSettings *_settings;
};

#endif // PLAINKEYSTORE_H
