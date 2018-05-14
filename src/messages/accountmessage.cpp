#include "accountmessage_p.h"
using namespace QtDataSync;

AccountMessage::AccountMessage(const QUuid &deviceId) :
	deviceId(deviceId)
{}

const QMetaObject *AccountMessage::getMetaObject() const
{
	return &staticMetaObject;
}
