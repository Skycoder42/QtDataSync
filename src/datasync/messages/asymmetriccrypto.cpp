#include "asymmetriccrypto_p.h"

#include <cryptopp/oids.h>

#include <qiodevicesource.h>
#include <qiodevicesink.h>

using namespace QtDataSync;
using namespace CryptoPP;

AsymmetricCrypto::AsymmetricCrypto(const QByteArray &signatureScheme, const QByteArray &encryptionScheme, CryptoPP::RandomNumberGenerator &rng, QObject *parent) :
	QObject(parent),
	_rng(rng)
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
		new CryptoPP::SignatureVerificationFilter(
			*(verifier.data()), nullptr,
			CryptoPP::SignatureVerificationFilter::THROW_EXCEPTION | CryptoPP::SignatureVerificationFilter::SIGNATURE_AT_END
		) // SignatureVerificationFilter
	); // QByteArraySource
}

QByteArray AsymmetricCrypto::sign(const PKCS8PrivateKey &key, const QByteArray &message)
{
	auto signer = _signature->signer(key);

	QByteArray signature;
	QByteArraySource (message, true,
		new CryptoPP::SignerFilter(_rng, *(signer.data()),
			new QByteArraySink(signature)
	   ) // SignerFilter
	); // QByteArraySource
	return signature;
}

QByteArray AsymmetricCrypto::encrypt(const X509PublicKey &key, const QByteArray &message)
{
}

QByteArray AsymmetricCrypto::decrypt(const PKCS8PrivateKey &key, const QByteArray &message)
{

}
