#include "identifymessage_p.h"

using namespace QtDataSync;

IdentifyMessage::IdentifyMessage() :
	nonce()
{}

IdentifyMessage IdentifyMessage::createRandom(CryptoPP::RandomNumberGenerator &rng)
{
	IdentifyMessage msg;
	msg.nonce.resize(NonceSize);
	rng.GenerateBlock((byte*)msg.nonce.data(), msg.nonce.size());
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
	if(message.nonce.size() < IdentifyMessage::NonceSize)
		stream.abortTransaction();
	else
		stream.commitTransaction();
	return stream;
}
