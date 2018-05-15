#include "loginmessage_p.h"
using namespace QtDataSync;

LoginMessage::LoginMessage(QUuid deviceId, QString deviceName, QByteArray nonce) :
	InitMessage(std::move(nonce)),
	deviceId(std::move(deviceId)),
	deviceName(std::move(deviceName))
{}

const QMetaObject *LoginMessage::getMetaObject() const
{
	return &staticMetaObject;
}
