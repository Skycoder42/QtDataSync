#include "asymmetriccrypto_p.h"

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

QSharedPointer<X509PublicKey> AsymmetricCrypto::readKey(bool signKey, const QByteArray &data) const
{
	auto key = signKey ?
				   _signature->createNullKey() :
				   _encryption->createNullKey();
	QByteArraySource source(data, true);
	key->Load(source);
	return key;
}

QByteArray AsymmetricCrypto::writeKey(const X509PublicKey &key) const
{
	QByteArray data;
	QByteArraySink sink(data);
	key.Save(sink);
	return data;
}

void AsymmetricCrypto::verify(const X509PublicKey &key, const QByteArray &message, const QByteArray &signature)
{
	auto verifier = _signature->verify(key);

	QByteArraySource (message + signature, true,
		new SignatureVerificationFilter(
			*(verifier.data()), nullptr,
			SignatureVerificationFilter::THROW_EXCEPTION | SignatureVerificationFilter::SIGNATURE_AT_END
		) // SignatureVerificationFilter
	); // QByteArraySource
}

QByteArray AsymmetricCrypto::sign(const PKCS8PrivateKey &key, RandomNumberGenerator &rng, const QByteArray &message)
{
	auto signer = _signature->sign(key);

	QByteArray signature;
	QByteArraySource (message, true,
		new SignerFilter(rng, *(signer.data()),
			new QByteArraySink(signature)
	   ) // SignerFilter
	); // QByteArraySource
	return signature;
}

QByteArray AsymmetricCrypto::encrypt(const X509PublicKey &key, CryptoPP::RandomNumberGenerator &rng, const QByteArray &message)
{
	auto encryptor = _encryption->encrypt(key);

	QByteArray cipher;
	QByteArraySource (message, true,
		new PK_EncryptorFilter(rng, *(encryptor.data()),
			new QByteArraySink(cipher)
	   ) // SignerFilter
	); // QByteArraySource
	return cipher;
}

QByteArray AsymmetricCrypto::decrypt(const PKCS8PrivateKey &key, RandomNumberGenerator &rng, const QByteArray &message)
{
	auto decryptor = _encryption->decrypt(key);

	QByteArray plain;
	QByteArraySource (message, true,
		new PK_DecryptorFilter(rng, *(decryptor.data()),
			new QByteArraySink(plain)
	   ) // SignerFilter
	); // QByteArraySource
	return plain;
}

void AsymmetricCrypto::setSignatureScheme(const QByteArray &name)
{
	auto stdStr = name.toStdString();
	if(stdStr.empty())
		_signature.reset();
	else if(stdStr == RsaSignScheme::StaticAlgorithmName())
		_signature.reset(new SignatureScheme<RsaSignScheme>());
	else if(stdStr == EccSignScheme::StaticAlgorithmName())
		_signature.reset(new SignatureScheme<EccSignScheme>());
	else
		throw Exception(Exception::NOT_IMPLEMENTED, "Signature Scheme \"" + stdStr + "\" not supported");
}

void AsymmetricCrypto::setEncryptionScheme(const QByteArray &name)
{
	auto stdStr = name.toStdString();
	if(stdStr.empty())
		_encryption.reset();
	else if(stdStr == RsaCryptScheme::StaticAlgorithmName())
		_encryption.reset(new EncryptionScheme<RsaCryptScheme>());
//	else if(stdStr == EccScheme::StaticAlgorithmName())
//		_encryption.reset(new EncryptionScheme<EccScheme>());
	else
		throw Exception(Exception::NOT_IMPLEMENTED, "Encryption Scheme \"" + stdStr + "\" not supported");
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
