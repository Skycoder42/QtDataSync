#include "devicesmessage_p.h"
using namespace QtDataSync;

ListDevicesMessage::ListDevicesMessage() {}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const ListDevicesMessage &message)
{
	Q_UNUSED(message);
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, ListDevicesMessage &message)
{
	Q_UNUSED(message);
	return stream;
}



DevicesMessage::DevicesMessage() :
	devices()
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const DevicesMessage &message)
{
	stream << message.devices;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, DevicesMessage &message)
{
	stream.startTransaction();
	stream >> message.devices;
	stream.commitTransaction();
	return stream;
}
