#include "asymmetriccrypto_p.h"

#include <QtCore/QCryptographicHash>

#include <qiodevicesource.h>
#include <qiodevicesink.h>

using namespace QtDataSync;
using namespace CryptoPP;

template <typename TScheme>
class SignatureScheme : public AsymmetricCrypto::Signature
{
public:
	QByteArray name() const override;
	QSharedPointer<X509PublicKey> createNullKey() const override;
	QSharedPointer<PK_Signer> sign(const PKCS8PrivateKey &pKey) const override;
	QSharedPointer<PK_Verifier> verify(const X509PublicKey &pubKey) const override;
};

template <typename TScheme>
class EncryptionScheme : public AsymmetricCrypto::Encryption
{
public:
	QByteArray name() const override;
	QSharedPointer<X509PublicKey> createNullKey() const override;
	QSharedPointer<PK_Encryptor> encrypt(const X509PublicKey &pubKey) const override;
	QSharedPointer<PK_Decryptor> decrypt(const PKCS8PrivateKey &pKey) const override;
};

// ------------- Main Implementation -------------

AsymmetricCrypto::AsymmetricCrypto(const QByteArray &signatureScheme, const QByteArray &encryptionScheme, QObject *parent) :
	AsymmetricCrypto(parent)
{
	setSignatureScheme(signatureScheme);
	setEncryptionScheme(encryptionScheme);
}

AsymmetricCrypto::AsymmetricCrypto(QObject *parent) :
	QObject(parent),
	_signature(nullptr),
	_encryption(nullptr)
{}

QByteArray AsymmetricCrypto::signatureScheme() const
{
	return _signature->name();
}

QByteArray AsymmetricCrypto::encryptionScheme() const
{
	return _encryption->name();
}

QByteArray AsymmetricCrypto::fingerprint(const X509PublicKey &signingKey, const X509PublicKey &encryptionKey) const
{
	QCryptographicHash hash(QCryptographicHash::Sha3_256);
	hash.addData(signatureScheme());
	hash.addData(writeKey(signingKey));
	hash.addData(encryptionScheme());
	hash.addData(writeKey(encryptionKey));
	return hash.result();
}

QSharedPointer<X509PublicKey> AsymmetricCrypto::readKey(bool signKey, CryptoPP::RandomNumberGenerator &rng, const QByteArray &data) const
{
	auto key = signKey ?
				   _signature->createNullKey() :
				   _encryption->createNullKey();
	QByteArraySource source(data, true);
	key->Load(source);
	if(!key->Validate(rng, 3))
	   throw Exception(Exception::INVALID_DATA_FORMAT, "Key failed validation");
	return key;
}

QByteArray AsymmetricCrypto::writeKey(const X509PublicKey &key) const
{
	QByteArray data;
	QByteArraySink sink(data);
	key.Save(sink);
	return data;
}

void AsymmetricCrypto::verify(const X509PublicKey &key, const QByteArray &message, const QByteArray &signature) const
{
	auto verifier = _signature->verify(key);

	QByteArraySource (message + signature, true,
		new SignatureVerificationFilter(
			*verifier, nullptr,
			SignatureVerificationFilter::THROW_EXCEPTION | SignatureVerificationFilter::SIGNATURE_AT_END
		) // SignatureVerificationFilter
	); // QByteArraySource
}

QByteArray AsymmetricCrypto::sign(const PKCS8PrivateKey &key, RandomNumberGenerator &rng, const QByteArray &message) const
{
	auto signer = _signature->sign(key);

	QByteArray signature;
	QByteArraySource (message, true,
		new SignerFilter(rng, *signer,
			new QByteArraySink(signature)
	   ) // SignerFilter
	); // QByteArraySource
	return signature;
}

QByteArray AsymmetricCrypto::encrypt(const X509PublicKey &key, CryptoPP::RandomNumberGenerator &rng, const QByteArray &message) const
{
	auto encryptor = _encryption->encrypt(key);

	QByteArray cipher;
	QByteArraySource (message, true,
		new PK_EncryptorFilter(rng, *encryptor,
			new QByteArraySink(cipher)
	   ) // SignerFilter
	); // QByteArraySource
	return cipher;
}

QByteArray AsymmetricCrypto::decrypt(const PKCS8PrivateKey &key, RandomNumberGenerator &rng, const QByteArray &message) const
{
	auto decryptor = _encryption->decrypt(key);

	QByteArray plain;
	QByteArraySource (message, true,
		new PK_DecryptorFilter(rng, *decryptor,
			new QByteArraySink(plain)
	   ) // SignerFilter
	); // QByteArraySource
	return plain;
}

void AsymmetricCrypto::setSignatureScheme(const QByteArray &name)
{
	auto stdStr = name.toStdString();
	if(stdStr == RsassScheme::StaticAlgorithmName())
		_signature.reset(new SignatureScheme<RsassScheme>());
	else if(stdStr == EcdsaScheme::StaticAlgorithmName())
		_signature.reset(new SignatureScheme<EcdsaScheme>());
	else if(stdStr == EcnrScheme::StaticAlgorithmName())
		_signature.reset(new SignatureScheme<EcnrScheme>());
	else
		throw Exception(Exception::NOT_IMPLEMENTED, "Signature Scheme \"" + stdStr + "\" not supported");
}

void AsymmetricCrypto::setEncryptionScheme(const QByteArray &name)
{
	auto stdStr = name.toStdString();
	if(stdStr == RsaesScheme::StaticAlgorithmName())
		_encryption.reset(new EncryptionScheme<RsaesScheme>());
#if CRYPTOPP_VERSION >= 600
	else if(stdStr == EciesScheme::StaticAlgorithmName())
		_encryption.reset(new EncryptionScheme<EciesScheme>());
#endif
	else
		throw Exception(Exception::NOT_IMPLEMENTED, "Encryption Scheme \"" + stdStr + "\" not supported");
}

void AsymmetricCrypto::resetSchemes()
{
	_signature.reset();
	_encryption.reset();
}

// ------------- AsymmetricCryptoInfo Implementation -------------

AsymmetricCryptoInfo::AsymmetricCryptoInfo(CryptoPP::RandomNumberGenerator &rng, const QByteArray &signatureScheme, const QByteArray &signatureKey, const QByteArray &encryptionScheme, const QByteArray &encryptionKey, QObject *parent) :
	AsymmetricCrypto(signatureScheme, encryptionScheme, parent),
	_signKey(readKey(true, rng, signatureKey)),
	_cryptKey(readKey(false, rng, encryptionKey))
{}

AsymmetricCryptoInfo::AsymmetricCryptoInfo(RandomNumberGenerator &rng, const QByteArray &encryptionScheme, const QByteArray &encryptionKey, QObject *parent) :
	AsymmetricCrypto(parent),
	_signKey(),
	_cryptKey()
{
	setEncryptionScheme(encryptionScheme);
	_cryptKey = readKey(false, rng, encryptionKey);
}

const X509PublicKey &AsymmetricCryptoInfo::signatureKey() const
{
	return *_signKey;
}

const X509PublicKey &AsymmetricCryptoInfo::encryptionKey() const
{
	return *_cryptKey;
}

QByteArray AsymmetricCryptoInfo::ownFingerprint() const
{
	return fingerprint(_signKey, _cryptKey);
}

void AsymmetricCryptoInfo::verify(const QByteArray &message, const QByteArray &signature) const
{
	return AsymmetricCrypto::verify(_signKey, message, signature);
}

QByteArray AsymmetricCryptoInfo::encrypt(RandomNumberGenerator &rng, const QByteArray &message) const
{
	return AsymmetricCrypto::encrypt(_cryptKey, rng, message);
}

// ------------- Generic Implementation -------------

template <typename TScheme>
QByteArray SignatureScheme<TScheme>::name() const
{
	return QByteArray::fromStdString(TScheme::StaticAlgorithmName());
}

template <typename TScheme>
QSharedPointer<X509PublicKey> SignatureScheme<TScheme>::createNullKey() const
{
	return QSharedPointer<typename TScheme::PublicKey>::create();
}

template <typename TScheme>
QSharedPointer<PK_Signer> SignatureScheme<TScheme>::sign(const PKCS8PrivateKey &pKey) const
{
	return QSharedPointer<typename TScheme::Signer>::create(pKey);
}

template <typename TScheme>
QSharedPointer<PK_Verifier> SignatureScheme<TScheme>::verify(const X509PublicKey &pubKey) const
{
	return QSharedPointer<typename TScheme::Verifier>::create(pubKey);
}



template <typename TScheme>
QByteArray EncryptionScheme<TScheme>::name() const
{
	return QByteArray::fromStdString(TScheme::StaticAlgorithmName());
}

template <typename TScheme>
QSharedPointer<X509PublicKey> EncryptionScheme<TScheme>::createNullKey() const
{
	return QSharedPointer<typename TScheme::PublicKey>::create();
}

template <typename TScheme>
QSharedPointer<PK_Encryptor> EncryptionScheme<TScheme>::encrypt(const X509PublicKey &pubKey) const
{
	return QSharedPointer<typename TScheme::Encryptor>::create(pubKey);
}

template <typename TScheme>
QSharedPointer<PK_Decryptor> EncryptionScheme<TScheme>::decrypt(const PKCS8PrivateKey &pKey) const
{
	return QSharedPointer<typename TScheme::Decryptor>::create(pKey);
}
