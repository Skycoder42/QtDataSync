#include "identifymessage_p.h"

using namespace QtDataSync;

IdentifyMessage::IdentifyMessage() :
	nonce(0)
{}

IdentifyMessage IdentifyMessage::createRandom(CryptoPP::RandomNumberGenerator &rng)
{
	IdentifyMessage msg;
	rng.GenerateBlock((byte*)&msg.nonce, sizeof(msg.nonce));
	return msg;
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
