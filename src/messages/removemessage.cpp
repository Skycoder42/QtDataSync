#include "removemessage_p.h"
using namespace QtDataSync;

RemoveMessage::RemoveMessage(const QUuid &deviceId) :
	deviceId(deviceId)
{}

const QMetaObject *RemoveMessage::getMetaObject() const
{
	return &staticMetaObject;
}



RemoveAckMessage::RemoveAckMessage(const QUuid &deviceId) :
	deviceId(deviceId)
{}

const QMetaObject *RemoveAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
