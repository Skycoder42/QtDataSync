#include "grantmessage_p.h"
using namespace QtDataSync;

GrantMessage::GrantMessage() :
	AccountMessage(),
	index(0),
	scheme(),
	secret()
{}

GrantMessage::GrantMessage(const QUuid &deviceId, const AcceptMessage &message) :
	AccountMessage(deviceId),
	index(message.index),
	scheme(message.scheme),
	secret(message.secret)
{}

const QMetaObject *GrantMessage::getMetaObject() const
{
	return &staticMetaObject;
}
