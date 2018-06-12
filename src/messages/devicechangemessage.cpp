#include "devicechangemessage_p.h"
using namespace QtDataSync;

DeviceChangeMessage::DeviceChangeMessage(QByteArray dataId, QUuid deviceId) :
	ChangeMessage{std::move(dataId)},
	deviceId{std::move(deviceId)}
{}

const QMetaObject *DeviceChangeMessage::getMetaObject() const
{
	return &staticMetaObject;
}



DeviceChangeAckMessage::DeviceChangeAckMessage(const DeviceChangeMessage &message) :
	ChangeAckMessage{message},
	deviceId{message.deviceId}
{}

const QMetaObject *DeviceChangeAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
