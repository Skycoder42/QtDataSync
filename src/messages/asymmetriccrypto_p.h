#ifndef QTDATASYNC_ASYMMETRICCRYPTO_P_H
#define QTDATASYNC_ASYMMETRICCRYPTO_P_H

#include <QtCore/QObject>

#include <cryptopp/rng.h>
#include <cryptopp/rsa.h>
#include <cryptopp/pssr.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/sha3.h>
#if CRYPTOPP_VERSION >= 800
#include <cryptopp/xed25519.h>
#endif

#include "message_p.h"

template <typename T>
class SignatureScheme;

namespace QtDataSync {

class Q_DATASYNC_EXPORT AsymmetricCrypto : public QObject
{
	Q_OBJECT

public:
	//not exported, only public for implementations
	class Scheme
	{
	public:
		inline virtual ~Scheme() = default;

		virtual QByteArray name() const = 0;
		virtual QSharedPointer<CryptoPP::X509PublicKey> createNullKey() const = 0;
	};

	//not exported, only public for implementations
	class Signature : public Scheme
	{
	public:
		virtual QSharedPointer<CryptoPP::PK_Signer> sign(const CryptoPP::PKCS8PrivateKey &pKey) const = 0;
		virtual QSharedPointer<CryptoPP::PK_Verifier> verify(const CryptoPP::X509PublicKey &pubKey) const = 0;
	};

	//not exported, only public for implementations
	class Encryption : public Scheme
	{
	public:
		virtual QSharedPointer<CryptoPP::PK_Encryptor> encrypt(const CryptoPP::X509PublicKey &pubKey) const = 0;
		virtual QSharedPointer<CryptoPP::PK_Decryptor> decrypt(const CryptoPP::PKCS8PrivateKey &pKey) const = 0;
	};

	explicit AsymmetricCrypto(const QByteArray &signatureScheme,
							  const QByteArray &encryptionScheme,
							  QObject *parent = nullptr);

	const static QByteArray Ed25519SchemeName;

	QByteArray signatureScheme() const;
	QByteArray encryptionScheme() const;

	QByteArray fingerprint(const CryptoPP::X509PublicKey &signingKey,
						   const CryptoPP::X509PublicKey &encryptionKey) const;
	inline QByteArray fingerprint(const QSharedPointer<CryptoPP::X509PublicKey> &signingKey,
								  const QSharedPointer<CryptoPP::X509PublicKey> &encryptionKey) const {
		return fingerprint(*signingKey, *encryptionKey);
	}

	QSharedPointer<CryptoPP::X509PublicKey> readKey(bool signKey, CryptoPP::RandomNumberGenerator &rng, const QByteArray &data) const;
	QByteArray writeKey(const CryptoPP::X509PublicKey &key) const;
	inline QByteArray writeKey(const QSharedPointer<CryptoPP::X509PublicKey> &key) const {
		return writeKey(*key);
	}

	void verify(const CryptoPP::X509PublicKey &key, const QByteArray &message, const QByteArray &signature) const;
	inline void verify(const QSharedPointer<CryptoPP::X509PublicKey> &key, const QByteArray &message, const QByteArray &signature) const {
		verify(*key, message, signature);
	}
	QByteArray sign(const CryptoPP::PKCS8PrivateKey &key, CryptoPP::RandomNumberGenerator &rng, const QByteArray &message) const;

	QByteArray encrypt(const CryptoPP::X509PublicKey &key, CryptoPP::RandomNumberGenerator &rng, const QByteArray &message) const;
	inline QByteArray encrypt(const QSharedPointer<CryptoPP::X509PublicKey> &key, CryptoPP::RandomNumberGenerator &rng, const QByteArray &message) const {
		return encrypt(*key, rng, message);
	}
	QByteArray decrypt(const CryptoPP::PKCS8PrivateKey &key, CryptoPP::RandomNumberGenerator &rng, const QByteArray &message) const;

protected:
	using RsassScheme = CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA3_512>;
	using EcdsaScheme = CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_512>;
	using EcnrScheme = CryptoPP::ECNR<CryptoPP::ECP, CryptoPP::SHA3_512>;
	using RsaesScheme = CryptoPP::RSAES<CryptoPP::OAEP<CryptoPP::SHA3_512>>;
#if CRYPTOPP_VERSION >= 600
	using EciesScheme = CryptoPP::ECIES<CryptoPP::ECP, CryptoPP::SHA3_512>;
#endif
#if CRYPTOPP_VERSION >= 800
	using Ed25519Scheme = CryptoPP::ed25519;
	friend class SignatureScheme<Ed25519Scheme>;
#endif

	explicit AsymmetricCrypto(QObject *parent = nullptr);

	void setSignatureScheme(const QByteArray &name);
	void setEncryptionScheme(const QByteArray &name);
	void resetSchemes();

private:
	QScopedPointer<Signature> _signature;
	QScopedPointer<Encryption> _encryption;
};

class Q_DATASYNC_EXPORT AsymmetricCryptoInfo : public AsymmetricCrypto
{
public:
	AsymmetricCryptoInfo(CryptoPP::RandomNumberGenerator &rng,
						 const QByteArray &signatureScheme,
						 const QByteArray &signatureKey,
						 const QByteArray &encryptionScheme,
						 const QByteArray &encryptionKey,
						 QObject *parent = nullptr);
	AsymmetricCryptoInfo(CryptoPP::RandomNumberGenerator &rng,
						 const QByteArray &encryptionScheme,
						 const QByteArray &encryptionKey,
						 QObject *parent = nullptr);

	const CryptoPP::X509PublicKey &signatureKey() const;
	const CryptoPP::X509PublicKey &encryptionKey() const;
	QByteArray ownFingerprint() const;

	void verify(const QByteArray &message, const QByteArray &signature) const;
	QByteArray encrypt(CryptoPP::RandomNumberGenerator &rng, const QByteArray &message) const;

private:
	QSharedPointer<CryptoPP::X509PublicKey> _signKey;
	QSharedPointer<CryptoPP::X509PublicKey> _cryptKey;
};

}

Q_DECLARE_METATYPE(CryptoPP::OID)

#endif // QTDATASYNC_ASYMMETRICCRYPTO_P_H
