#include "registermessage_p.h"

#include <qiodevicesink.h>
#include <qiodevicesource.h>

using namespace QtDataSync;

RegisterMessage::RegisterMessage() :
	keyAlgorithm(),
	pubKey(),
	deviceName()
{}

void RegisterMessage::getKey(CryptoPP::X509PublicKey &key) const
{
	QByteArraySource source(pubKey, true);
	key.BERDecodePublicKey(source, false, 0);
}

void RegisterMessage::setKey(const CryptoPP::X509PublicKey &key)
{
	pubKey.clear();
	QByteArraySink sink(pubKey);
	key.DEREncodePublicKey(sink);
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const RegisterMessage &message)
{
	stream << QByteArray::fromStdString(message.keyAlgorithm)
		   << message.pubKey
		   << message.deviceName;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, RegisterMessage &message)
{
	stream.startTransaction();
	QByteArray keyAlg;
	stream >> keyAlg
		   >> message.pubKey
		   >> message.deviceName;
	message.keyAlgorithm = keyAlg.toStdString();
	stream.commitTransaction();
	return stream;
}

QDebug QtDataSync::operator<<(QDebug debug, const RegisterMessage &message)
{
	QDebugStateSaver saver(debug);
	debug << QByteArray::fromStdString(message.keyAlgorithm) << message.deviceName;
	return debug;
}
