#include "removemessage_p.h"
using namespace QtDataSync;

RemoveMessage::RemoveMessage(QUuid deviceId) :
	deviceId{deviceId}
{}

const QMetaObject *RemoveMessage::getMetaObject() const
{
	return &staticMetaObject;
}



RemoveAckMessage::RemoveAckMessage(QUuid deviceId) :
	deviceId{deviceId}
{}

const QMetaObject *RemoveAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
