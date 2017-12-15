#ifndef CRYPTOCONTROLLER_P_H
#define CRYPTOCONTROLLER_P_H

#include <QtCore/QObject>
#include <QtCore/QUuid>

#include <cryptopp/osrng.h>
#include <cryptopp/oids.h>

#include "qtdatasync_global.h"
#include "defaults.h"
#include "keystore.h"
#include "controller_p.h"

#include "message_p.h"
#include "asymmetriccrypto_p.h"

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

class Q_DATASYNC_EXPORT ClientCrypto : public AsymmetricCrypto
{
	Q_OBJECT

public:
	class Q_DATASYNC_EXPORT KeyScheme
	{
	public:
		inline virtual ~KeyScheme() = default;

		virtual QByteArray name() const = 0;
		virtual void createPrivateKey(CryptoPP::RandomNumberGenerator &rng, const QVariant &keyParam) = 0;
		virtual CryptoPP::PKCS8PrivateKey &privateKeyRef() = 0;
		virtual QSharedPointer<CryptoPP::X509PublicKey> createPublicKey() const = 0;
	};

	ClientCrypto(QObject *parent = nullptr);

	void generate(Setup::SignatureScheme signScheme,
				  const QVariant &signKeyParam,
				  Setup::EncryptionScheme cryptScheme,
				  const QVariant &cryptKeyParam);

	void load(const QByteArray &signScheme,
			  const QByteArray &signKey,
			  const QByteArray &cryptScheme,
			  const QByteArray &cryptKey);

	CryptoPP::RandomNumberGenerator &rng();

	QSharedPointer<CryptoPP::X509PublicKey> readKey(bool signKey, const QByteArray &data);

	QSharedPointer<CryptoPP::X509PublicKey> signKey() const;
	QByteArray writeSignKey() const;
	QSharedPointer<CryptoPP::X509PublicKey> cryptKey() const;
	QByteArray writeCryptKey() const;

	const CryptoPP::PKCS8PrivateKey &privateSignKey() const;
	QByteArray savePrivateSignKey() const;
	const CryptoPP::PKCS8PrivateKey &privateCryptKey() const;
	QByteArray savePrivateCryptKey() const;

	QByteArray sign(const QByteArray &message);

	QByteArray encrypt(const CryptoPP::X509PublicKey &key, const QByteArray &message);
	inline QByteArray encrypt(const QSharedPointer<CryptoPP::X509PublicKey> &key, const QByteArray &message) {
		return encrypt(*(key.data()), message);
	}
	QByteArray decrypt(const QByteArray &message);

	static CryptoPP::OID curveId(Setup::EllipticCurve curve);

private:
	CryptoPP::AutoSeededRandomPool _rng;
	QScopedPointer<KeyScheme> _signKey;
	QScopedPointer<KeyScheme> _cryptKey;

	void setSignatureKey(const QByteArray &name);
	void setSignatureKey(Setup::SignatureScheme scheme);
	void setEncryptionKey(const QByteArray &name);
	void setEncryptionKey(Setup::EncryptionScheme scheme);

	void loadKey(CryptoPP::PKCS8PrivateKey &key, const QByteArray &data);
	QByteArray saveKey(const CryptoPP::PKCS8PrivateKey &key) const;
};

class Q_DATASYNC_EXPORT CryptoController : public Controller
{
	Q_OBJECT

public:
	explicit CryptoController(const Defaults &defaults, QObject *parent = nullptr);

	static QStringList allKeystoreKeys();

	void initialize() final;
	void finalize() final;

	ClientCrypto *crypto() const;
	QByteArray fingerprint() const;

	bool canAccessStore() const;
	void loadKeyMaterial(const QUuid &deviceId);

	void createPrivateKeys(const QByteArray &nonce);
	void storePrivateKeys(const QUuid &deviceId);

	template <typename TMessage>
	QByteArray serializeSignedMessage(const TMessage &message);

private:
	static const QString keySignScheme;
	static const QString keyCryptScheme;
	static const QString keySignTemplate;
	static const QString keyCryptTemplate;

	QPointer<KeyStore> _keyStore;
	ClientCrypto *_crypto;

	mutable QByteArray _fingerprint;

	void ensureStoreAccess() const;
};

// ------------- Generic Implementation -------------

template<typename TMessage>
QByteArray CryptoController::serializeSignedMessage(const TMessage &message)
{
	try {
		return QtDataSync::serializeSignedMessage<TMessage>(message,
															_crypto->privateSignKey(),
															_crypto->rng(),
															_crypto);
	} catch(CryptoPP::Exception &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to sign message"),
							  e);
	}
}
}

#endif // CRYPTOCONTROLLER_P_H
