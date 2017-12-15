#include "loginmessage_p.h"
using namespace QtDataSync;

LoginMessage::LoginMessage(const QUuid &deviceId, const QString &name, const QByteArray &nonce) :
	deviceId(deviceId),
	name(name),
	nonce(nonce)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const LoginMessage &message)
{
	stream << message.deviceId
		   << message.name
		   << message.nonce;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, LoginMessage &message)
{
	stream.startTransaction();
	stream >> message.deviceId
		   >> message.name
		   >> message.nonce;
	stream.commitTransaction();
	return stream;
}
