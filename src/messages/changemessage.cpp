#include "changemessage_p.h"
using namespace QtDataSync;

ChangeMessage::ChangeMessage(QByteArray dataId) :
	dataId{std::move(dataId)}
{}

const QMetaObject *ChangeMessage::getMetaObject() const
{
	return &staticMetaObject;
}



ChangeAckMessage::ChangeAckMessage(const ChangeMessage &message) :
	dataId{message.dataId}
{}

const QMetaObject *ChangeAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
