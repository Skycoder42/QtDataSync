#include "syncmessage_p.h"

#include <QtCore/QMetaEnum>

using namespace QtDataSync;

SyncMessage::SyncMessage(Action action) :
	action(action)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const SyncMessage &message)
{
	stream << (int)message.action;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, SyncMessage &message)
{
	stream.startTransaction();
	stream >> (int&)message.action;
	if(QMetaEnum::fromType<SyncMessage::Action>().valueToKey(message.action))
		stream.commitTransaction();
	else
		stream.abortTransaction();
	return stream;
}
