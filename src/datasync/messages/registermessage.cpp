#include "registermessage_p.h"

using namespace QtDataSync;

RegisterBaseMessage::RegisterBaseMessage() :
	IdentifyMessage(),
	signAlgorithm(),
	signKey(),
	cryptAlgorithm(),
	cryptKey(),
	deviceName()
{}

RegisterBaseMessage::RegisterBaseMessage(const QString &deviceName, const QByteArray &nonce, const QSharedPointer<CryptoPP::X509PublicKey> &signKey, const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey, AsymmetricCrypto *crypto) :
	IdentifyMessage(nonce),
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

QDataStream &QtDataSync::operator<<(QDataStream &stream, const RegisterBaseMessage &message)
{
	stream << (IdentifyMessage)message
		   << message.signAlgorithm
		   << message.signKey
		   << message.cryptAlgorithm
		   << message.cryptKey
		   << message.deviceName;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, RegisterBaseMessage &message)
{
	stream.startTransaction();
	stream >> (IdentifyMessage&)message
		   >> message.signAlgorithm
		   >> message.signKey
		   >> message.cryptAlgorithm
		   >> message.cryptKey
		   >> message.deviceName;
	stream.commitTransaction();
	return stream;
}



RegisterMessage::RegisterMessage() :
	RegisterBaseMessage()
{}

RegisterMessage::RegisterMessage(const QString &deviceName, const QByteArray &nonce, const QSharedPointer<CryptoPP::X509PublicKey> &signKey, const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey, AsymmetricCrypto *crypto) :
	RegisterBaseMessage(deviceName,
						nonce,
						signKey,
						cryptKey,
						crypto)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const RegisterMessage &message)
{
	stream << (RegisterBaseMessage)message;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, RegisterMessage &message)
{
	stream.startTransaction();
	stream >> (RegisterBaseMessage&)message;
	stream.commitTransaction();
	return stream;
}
