#include "accountmessage_p.h"
using namespace QtDataSync;

AccountMessage::AccountMessage(const QUuid &deviceId) :
	deviceId(deviceId)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const AccountMessage &message)
{
	stream << message.deviceId;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, AccountMessage &message)
{
	stream.startTransaction();
	stream >> message.deviceId;
	stream.commitTransaction();
	return stream;
}
