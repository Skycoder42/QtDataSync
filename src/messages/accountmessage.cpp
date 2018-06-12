#include "accountmessage_p.h"
using namespace QtDataSync;

AccountMessage::AccountMessage(QUuid deviceId) :
	deviceId{std::move(deviceId)}
{}

const QMetaObject *AccountMessage::getMetaObject() const
{
	return &staticMetaObject;
}
