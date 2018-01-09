#include "loginmessage_p.h"
using namespace QtDataSync;

LoginMessage::LoginMessage(const QUuid &deviceId, const QString &name, const QByteArray &nonce) :
	InitMessage(nonce),
	deviceId(deviceId),
	name(name)
{}

const QMetaObject *LoginMessage::getMetaObject() const
{
	return &staticMetaObject;
}
