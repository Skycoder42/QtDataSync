#include "loginmessage_p.h"
using namespace QtDataSync;

LoginMessage::LoginMessage(const QUuid &deviceId, const QString &deviceName, const QByteArray &nonce) :
	InitMessage(nonce),
	deviceId(deviceId),
	deviceName(deviceName)
{}

const QMetaObject *LoginMessage::getMetaObject() const
{
	return &staticMetaObject;
}
