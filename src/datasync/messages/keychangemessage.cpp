#include "keychangemessage_p.h"
using namespace QtDataSync;

KeyChangeMessage::KeyChangeMessage(quint32 nextIndex) :
	nextIndex(nextIndex)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const KeyChangeMessage &message)
{
	stream << message.nextIndex;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, KeyChangeMessage &message)
{
	stream.startTransaction();//TODO remove where uneccessary?
	stream >> message.nextIndex;
	stream.commitTransaction();
	return stream;
}
