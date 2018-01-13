#ifndef MACKEYCHAINKEYSTORE_H
#define MACKEYCHAINKEYSTORE_H

#include <tuple>

#include <QtDataSync/keystore.h>
#include <QtDataSync/logger.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

class MacKeychainKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit MacKeychainKeyStore(const QtDataSync::Defaults &defaults, QObject *parent = nullptr);

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
	QtDataSync::Logger *_logger;

	static QByteArray serviceName();
	static QString errorMsg(OSStatus status);

	SecKeychainItemRef loadItem(const QString &key) const;
};

#endif // MACKEYCHAINKEYSTORE_H
