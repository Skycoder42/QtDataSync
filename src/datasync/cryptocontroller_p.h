#ifndef CRYPTOCONTROLLER_P_H
#define CRYPTOCONTROLLER_P_H

#include <QtCore/QObject>
#include <QtCore/QUuid>

#include <cryptopp/osrng.h>
#include <cryptopp/rsa.h>
#include <cryptopp/pssr.h>
#include <cryptopp/sha3.h>

#include "qtdatasync_global.h"
#include "defaults.h"
#include "keystore.h"
#include "controller_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT CryptoException : public Exception
{
public:
	CryptoException(const Defaults &defaults,
					const QString &message,
					const CryptoPP::Exception &cExcept);

	CryptoPP::Exception cryptoPPException() const;
	QString error() const;
	CryptoPP::Exception::ErrorType type() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	CryptoException(const CryptoException * const other);
	CryptoPP::Exception _exception;
};

class Q_DATASYNC_EXPORT CryptoController : public Controller
{
	Q_OBJECT

public:
	explicit CryptoController(const Defaults &defaults, QObject *parent = nullptr);

	static QStringList allKeys();

	void initialize() final;
	void finalize() final;

	bool canAccess() const;
	bool loadKeyMaterial(const QUuid &userId);

	void createPrivateKey(quint32 nonce);
	CryptoPP::RSA::PublicKey publicKey() const;

	void signMessage(QByteArray &message);

private:

	static const QString keyPKeyTemplate;

	KeyStore *_keyStore;

	CryptoPP::AutoSeededRandomPool _rng;
	CryptoPP::RSA::PrivateKey _privateKey;
	QByteArray _fingerprint;

	void updateFingerprint();
};

}

#endif // CRYPTOCONTROLLER_P_H
