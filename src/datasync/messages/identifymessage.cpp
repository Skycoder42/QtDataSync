#include "identifymessage_p.h"

using namespace QtDataSync;

IdentifyMessage::IdentifyMessage(quint32 nounce) :
	nonce(nounce)
{}

IdentifyMessage IdentifyMessage::createRandom(CryptoPP::RandomNumberGenerator &rng)
{
	return (quint32)rng.GenerateWord32();
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const IdentifyMessage &message)
{
	stream << message.nonce;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, IdentifyMessage &message)
{
	stream.startTransaction();
	stream >> message.nonce;
	stream.commitTransaction();
	return stream;
}

QDebug QtDataSync::operator<<(QDebug debug, const IdentifyMessage &message)
{
	QDebugStateSaver saver(debug);
	debug.nospace() << "IdentifyMessage{"
					<< "nonce: 0x" << hex << message.nonce
					<< "}";
	return debug;
}
