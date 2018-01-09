#include "devicechangemessage_p.h"
using namespace QtDataSync;

DeviceChangeMessage::DeviceChangeMessage(const QByteArray &dataId, const QUuid &deviceId) :
	ChangeMessage(dataId),
	deviceId(deviceId)
{}

const QMetaObject *DeviceChangeMessage::getMetaObject() const
{
	return &staticMetaObject;
}



DeviceChangeAckMessage::DeviceChangeAckMessage(const DeviceChangeMessage &message) :
	ChangeAckMessage(message),
	deviceId(message.deviceId)
{}

const QMetaObject *DeviceChangeAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
