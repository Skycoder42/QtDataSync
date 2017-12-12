#include "registermessage_p.h"

using namespace QtDataSync;

RegisterMessage::RegisterMessage() :
	keyAlgorithm(),
	pubKey(),
	deviceName()
{}

RegisterMessage::RegisterMessage(const QString &deviceName, const CryptoPP::X509PublicKey &pubKey, AsymmetricCrypto *crypto) :
	keyAlgorithm(crypto->signatureScheme()),
	pubKey(crypto->writeKey(pubKey)),
	deviceName(deviceName)
{}

AsymmetricCrypto *RegisterMessage::createCrypto(QObject *parent)
{
	return new AsymmetricCrypto(keyAlgorithm, {}, parent);
}

QSharedPointer<CryptoPP::X509PublicKey> RegisterMessage::getKey(AsymmetricCrypto *crypto)
{
	if(crypto->signatureScheme() != keyAlgorithm)
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_ARGUMENT, "Signature scheme of passed crypto object does not match the messages scheme");
	return crypto->readKey(true, pubKey);
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const RegisterMessage &message)
{
	stream << message.keyAlgorithm
		   << message.pubKey
		   << message.deviceName;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, RegisterMessage &message)
{
	stream.startTransaction();
	stream >> message.keyAlgorithm
		   >> message.pubKey
		   >> message.deviceName;
	stream.commitTransaction();
	return stream;
}

QDebug QtDataSync::operator<<(QDebug debug, const RegisterMessage &message)
{
	QDebugStateSaver saver(debug);
	debug << message.keyAlgorithm << message.deviceName;
	return debug;
}
