#ifndef ASYMMETRICCRYPTO_P_H
#define ASYMMETRICCRYPTO_P_H

#include "message_p.h"

#include <QtCore/QObject>

#include <cryptopp/rsa.h>
#include <cryptopp/pssr.h>
#include <cryptopp/sha3.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/rng.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT AsymmetricCrypto : public QObject
{
	Q_OBJECT

public:
	explicit AsymmetricCrypto(const QByteArray &signatureScheme,
							  const QByteArray &encryptionScheme,
							  QObject *parent = nullptr);

	QByteArray signatureScheme() const;
	QByteArray encryptionScheme() const;

	QSharedPointer<CryptoPP::X509PublicKey> readKey(bool signKey, const QByteArray &data) const;
	QByteArray writeKey(const CryptoPP::X509PublicKey &key) const;
	inline QByteArray writeKey(const QSharedPointer<CryptoPP::X509PublicKey> &key) {
		return writeKey(*(key.data()));
	}

	void verify(const CryptoPP::X509PublicKey &key, const QByteArray &message, const QByteArray &signature);
	inline void verify(const QSharedPointer<CryptoPP::X509PublicKey> &key, const QByteArray &message, const QByteArray &signature) {
		verify(*(key.data()), message, signature);
	}
	QByteArray sign(const CryptoPP::PKCS8PrivateKey &key, CryptoPP::RandomNumberGenerator &rng, const QByteArray &message);

	QByteArray encrypt(const CryptoPP::X509PublicKey &key, CryptoPP::RandomNumberGenerator &rng, const QByteArray &message);
	inline QByteArray encrypt(const QSharedPointer<CryptoPP::X509PublicKey> &key, CryptoPP::RandomNumberGenerator &rng, const QByteArray &message) {
		return encrypt(*(key.data()), rng, message);
	}
	QByteArray decrypt(const CryptoPP::PKCS8PrivateKey &key, CryptoPP::RandomNumberGenerator &rng, const QByteArray &message);


private:
	class Q_DATASYNC_EXPORT Scheme
	{
	public:
		inline virtual ~Scheme() = default;

		virtual QByteArray name() const = 0;
		virtual QSharedPointer<CryptoPP::X509PublicKey> createNullKey() const = 0;
	};

	class Q_DATASYNC_EXPORT Signature : public Scheme
	{
	public:
		virtual QSharedPointer<CryptoPP::PK_Signer> sign(const CryptoPP::PKCS8PrivateKey &pKey) const = 0;
		virtual QSharedPointer<CryptoPP::PK_Verifier> verify(const CryptoPP::X509PublicKey &pubKey) const = 0;
	};

	class Q_DATASYNC_EXPORT Encryption : public Scheme
	{
	public:
		virtual QSharedPointer<CryptoPP::PK_Encryptor> encrypt(const CryptoPP::X509PublicKey &pubKey) const = 0;
		virtual QSharedPointer<CryptoPP::PK_Decryptor> decrypt(const CryptoPP::PKCS8PrivateKey &pKey) const = 0;
	};

	QScopedPointer<Signature> _signature;
	QScopedPointer<Encryption> _encryption;

	static void setSignatureScheme(QScopedPointer<Signature> &ptr, const QByteArray &name);
	static void setEncryptionScheme(QScopedPointer<Encryption> &ptr, const QByteArray &name);
};

}

Q_DECLARE_METATYPE(CryptoPP::OID)

#endif // ASYMMETRICCRYPTO_P_H
