#include "identifymessage_p.h"

#include <QtCore/QThreadStorage>

#include <cryptopp/osrng.h>

using namespace QtDataSync;

static QThreadStorage<CryptoPP::AutoSeededRandomPool> rng;

IdentifyMessage::IdentifyMessage(quint32 nounce) :
	nonce(nounce)
{}

IdentifyMessage IdentifyMessage::createRandom()
{
	return (quint32)rng.localData().GenerateWord32();
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
