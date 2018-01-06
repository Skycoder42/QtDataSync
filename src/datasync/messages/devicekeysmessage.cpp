#include "devicekeysmessage_p.h"
using namespace QtDataSync;

DeviceKeysMessage::DeviceKeysMessage(quint32 keyIndex, const QList<DeviceKey> &devices) :
	keyIndex(keyIndex),
	devices(devices)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const DeviceKeysMessage &message)
{
	stream << message.keyIndex
		   << message.devices;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, DeviceKeysMessage &message)
{
	stream.startTransaction();
	stream >> message.keyIndex
		   >> message.devices;
	stream.commitTransaction();
	return stream;
}
