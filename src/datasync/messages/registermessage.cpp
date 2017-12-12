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

RegisterMessage::RegisterMessage(const QString &deviceName, quint32 nonce, const QSharedPointer<CryptoPP::X509PublicKey> &signKey, const QSharedPointer<CryptoPP::X509PublicKey> &cryptKey, AsymmetricCrypto *crypto) :
	signAlgorithm(crypto->signatureScheme()),
	signKey(crypto->writeKey(signKey)),
	cryptAlgorithm(crypto->encryptionScheme()),
	cryptKey(crypto->writeKey(cryptKey)),
	deviceName(deviceName),
	nonce(nonce)
{}

AsymmetricCrypto *RegisterMessage::createCrypto(QObject *parent)
{
	return new AsymmetricCrypto(signAlgorithm, cryptAlgorithm, parent);
}

QSharedPointer<CryptoPP::X509PublicKey> RegisterMessage::getSignKey(CryptoPP::RandomNumberGenerator &rng, AsymmetricCrypto *crypto)
{
	if(crypto->signatureScheme() != signAlgorithm)
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_ARGUMENT, "Signature scheme of passed crypto object does not match the messages scheme");
	return crypto->readKey(true, rng, signKey);
}

QSharedPointer<CryptoPP::X509PublicKey> RegisterMessage::getCryptKey(CryptoPP::RandomNumberGenerator &rng, AsymmetricCrypto *crypto)
{
	if(crypto->encryptionScheme() != cryptAlgorithm)
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_ARGUMENT, "Encryption scheme of passed crypto object does not match the messages scheme");
	return crypto->readKey(false, rng, cryptKey);
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

QDebug QtDataSync::operator<<(QDebug debug, const RegisterMessage &message)
{
	QDebugStateSaver saver(debug);
	debug << message.deviceName << message.signAlgorithm << message.cryptAlgorithm;
	return debug;
}
