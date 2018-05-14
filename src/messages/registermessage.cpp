#include "registermessage_p.h"

using namespace QtDataSync;

RegisterBaseMessage::RegisterBaseMessage() :
	InitMessage(),
	signAlgorithm(),
	signKey(),
	cryptAlgorithm(),
	cryptKey(),
	deviceName()
{}

RegisterBaseMessage::RegisterBaseMessage(const QString &deviceName, const QByteArray &nonce, const QSharedPointer<CryptoPP::X509PublicKey> &signKey, const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey, AsymmetricCrypto *crypto) :
	InitMessage(nonce),
	signAlgorithm(crypto->signatureScheme()),
	signKey(crypto->writeKey(signKey)),
	cryptAlgorithm(crypto->encryptionScheme()),
	cryptKey(crypto->writeKey(cryptKey)),
	deviceName(deviceName)
{}

AsymmetricCryptoInfo *RegisterBaseMessage::createCryptoInfo(CryptoPP::RandomNumberGenerator &rng, QObject *parent) const
{
	return new AsymmetricCryptoInfo(rng,
									signAlgorithm,
									signKey,
									cryptAlgorithm,
									cryptKey,
									parent);
}

const QMetaObject *RegisterBaseMessage::getMetaObject() const
{
	return &staticMetaObject;
}



RegisterMessage::RegisterMessage() :
	RegisterBaseMessage(),
	cmac()
{}

RegisterMessage::RegisterMessage(const QString &deviceName, const QByteArray &nonce, const QSharedPointer<CryptoPP::X509PublicKey> &signKey, const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey, AsymmetricCrypto *crypto, const QByteArray &cmac) :
	RegisterBaseMessage(deviceName,
						nonce,
						signKey,
						cryptKey,
						crypto),
	cmac(cmac)
{}

const QMetaObject *RegisterMessage::getMetaObject() const
{
	return &staticMetaObject;
}
