#include "registermessage_p.h"

using namespace QtDataSync;

RegisterBaseMessage::RegisterBaseMessage() = default;

RegisterBaseMessage::RegisterBaseMessage(QString deviceName, QByteArray nonce, const QSharedPointer<CryptoPP::X509PublicKey> &signKey, const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey, AsymmetricCrypto *crypto) :
	InitMessage(std::move(nonce)),
	signAlgorithm(crypto->signatureScheme()),
	signKey(crypto->writeKey(signKey)),
	cryptAlgorithm(crypto->encryptionScheme()),
	cryptKey(crypto->writeKey(cryptKey)),
	deviceName(std::move(deviceName))
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



RegisterMessage::RegisterMessage() = default;

RegisterMessage::RegisterMessage(const QString &deviceName, const QByteArray &nonce, const QSharedPointer<CryptoPP::X509PublicKey> &signKey, const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey, AsymmetricCrypto *crypto, QByteArray cmac) :
	RegisterBaseMessage(deviceName,
						nonce,
						signKey,
						cryptKey,
						crypto),
	cmac(std::move(cmac))
{}

const QMetaObject *RegisterMessage::getMetaObject() const
{
	return &staticMetaObject;
}
