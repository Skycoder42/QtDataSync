#include "removemessage_p.h"
using namespace QtDataSync;

RemoveMessage::RemoveMessage(QUuid deviceId) :
	deviceId(std::move(deviceId))
{}

const QMetaObject *RemoveMessage::getMetaObject() const
{
	return &staticMetaObject;
}



RemoveAckMessage::RemoveAckMessage(QUuid deviceId) :
	deviceId(std::move(deviceId))
{}

const QMetaObject *RemoveAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
