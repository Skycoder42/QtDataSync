#include "devicechangemessage_p.h"
using namespace QtDataSync;

DeviceChangeMessage::DeviceChangeMessage(const QByteArray &dataId, const QUuid &deviceId) :
	ChangeMessage(dataId),
	deviceId(deviceId)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const DeviceChangeMessage &message)
{
	stream << static_cast<ChangeMessage>(message)
		   << message.deviceId;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, DeviceChangeMessage &message)
{
	stream.startTransaction();
	stream >> static_cast<ChangeMessage&>(message)
		   >> message.deviceId;
	stream.commitTransaction();
	return stream;
}



DeviceChangeAckMessage::DeviceChangeAckMessage(const DeviceChangeMessage &message) :
	ChangeAckMessage(message),
	deviceId(message.deviceId)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const DeviceChangeAckMessage &message)
{
	stream << static_cast<ChangeAckMessage>(message)
		   << message.deviceId;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, DeviceChangeAckMessage &message)
{
	stream.startTransaction();
	stream >> static_cast<ChangeAckMessage&>(message)
		   >> message.deviceId;
	stream.commitTransaction();
	return stream;
}
