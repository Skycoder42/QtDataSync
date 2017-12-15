#include "registermessage_p.h"

using namespace QtDataSync;

RegisterMessage::RegisterMessage() :
	signAlgorithm(),
	signKey(),
	cryptAlgorithm(),
	cryptKey(),
	deviceName(),
	nonce()
{}

RegisterMessage::RegisterMessage(const QString &deviceName, const QByteArray &nonce, const QSharedPointer<CryptoPP::X509PublicKey> &signKey, const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey, AsymmetricCrypto *crypto) :
	signAlgorithm(crypto->signatureScheme()),
	signKey(crypto->writeKey(signKey)),
	cryptAlgorithm(crypto->encryptionScheme()),
	cryptKey(crypto->writeKey(cryptKey)),
	deviceName(deviceName),
	nonce(nonce)
{}

AsymmetricCryptoInfo *RegisterMessage::createCryptoInfo(CryptoPP::RandomNumberGenerator &rng, QObject *parent) const
{
	return new AsymmetricCryptoInfo(rng,
									signAlgorithm,
									signKey,
									cryptAlgorithm,
									cryptKey,
									parent);
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const RegisterMessage &message)
{
	stream << message.signAlgorithm
		   << message.signKey
		   << message.cryptAlgorithm
		   << message.cryptKey
		   << message.deviceName
		   << message.nonce;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, RegisterMessage &message)
{
	stream.startTransaction();
	stream >> message.signAlgorithm
		   >> message.signKey
		   >> message.cryptAlgorithm
		   >> message.cryptKey
		   >> message.deviceName
		   >> message.nonce;
	stream.commitTransaction();
	return stream;
}
