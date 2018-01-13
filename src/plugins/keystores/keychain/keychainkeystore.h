#ifndef KEYCHAINKEYSTORE_H
#define KEYCHAINKEYSTORE_H

#include <tuple>

#include <QtDataSync/keystore.h>
#include <QtDataSync/logger.h>

#include <CoreFoundation/CoreFoundation.h>

class KeychainKeyStore : public QtDataSync::KeyStore
{
	Q_OBJECT

public:
	explicit KeychainKeyStore(const QtDataSync::Defaults &defaults, QObject *parent = nullptr);

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

	static QString serviceName();
	static QString errorMsg(OSStatus status);
};

#endif // KEYCHAINKEYSTORE_H
