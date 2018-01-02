#include "removemessage_p.h"
using namespace QtDataSync;

RemoveMessage::RemoveMessage(const QUuid &deviceId) :
	deviceId(deviceId)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const RemoveMessage &message)
{
	stream << message.deviceId;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, RemoveMessage &message)
{
	stream.startTransaction();
	stream >> message.deviceId;
	stream.commitTransaction();
	return stream;
}



RemovedMessage::RemovedMessage(const QUuid &deviceId) :
	deviceId(deviceId)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const RemovedMessage &message)
{
	stream << message.deviceId;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, RemovedMessage &message)
{
	stream.startTransaction();
	stream >> message.deviceId;
	stream.commitTransaction();
	return stream;
}
