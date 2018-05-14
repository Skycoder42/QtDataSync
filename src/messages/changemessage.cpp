#include "changemessage_p.h"
using namespace QtDataSync;

ChangeMessage::ChangeMessage(const QByteArray &dataId) :
	dataId(dataId),
	keyIndex(0),
	salt(),
	data()
{}

const QMetaObject *ChangeMessage::getMetaObject() const
{
	return &staticMetaObject;
}



ChangeAckMessage::ChangeAckMessage(const ChangeMessage &message) :
	dataId(message.dataId)
{}

const QMetaObject *ChangeAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
