#ifndef ASYMMETRICCRYPTO_P_H
#define ASYMMETRICCRYPTO_P_H

#include "message_p.h"

#include <QtCore/QObject>

#include <cryptopp/rng.h>
#include <cryptopp/rsa.h>
#include <cryptopp/pssr.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/sha3.h>

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
	inline QByteArray writeKey(const QSharedPointer<CryptoPP::X509PublicKey> &key) const {
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

protected:
	typedef CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA3_256> RsaSignScheme;
	typedef CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256> EccSignScheme;
	typedef CryptoPP::RSAES<CryptoPP::OAEP<CryptoPP::SHA3_256>> RsaCryptScheme;
	//TODO cryptopp 6.0: typedef CryptoPP::ECIES<CryptoPP::ECP, CryptoPP::SHA3_256> EccCryptScheme;

	explicit AsymmetricCrypto(QObject *parent = nullptr);

	void setSignatureScheme(const QByteArray &name);
	void setEncryptionScheme(const QByteArray &name);

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
};

}

Q_DECLARE_METATYPE(CryptoPP::OID)

#endif // ASYMMETRICCRYPTO_P_H
