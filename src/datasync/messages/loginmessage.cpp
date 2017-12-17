#include "loginmessage_p.h"
using namespace QtDataSync;

LoginMessage::LoginMessage(const QUuid &deviceId, const QString &name, const QByteArray &nonce) :
	IdentifyMessage(nonce),
	deviceId(deviceId),
	name(name)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const LoginMessage &message)
{
	stream << (IdentifyMessage)message
		   << message.deviceId
		   << message.name;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, LoginMessage &message)
{
	stream.startTransaction();
	stream >> (IdentifyMessage&)message
		   >> message.deviceId
		   >> message.name;
	stream.commitTransaction();
	return stream;
}
