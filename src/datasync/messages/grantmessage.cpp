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


QDataStream &QtDataSync::operator<<(QDataStream &stream, const QtDataSync::GrantMessage &message)
{
	stream << static_cast<AccountMessage>(message)
		   << message.index
		   << message.scheme
		   << message.secret;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, QtDataSync::GrantMessage &message)
{
	stream.startTransaction();
	stream >> static_cast<AccountMessage&>(message)
		   >> message.index
		   >> message.scheme
		   >> message.secret;
	stream.commitTransaction();
	return stream;
}
