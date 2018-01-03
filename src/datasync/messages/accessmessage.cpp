#include "accessmessage_p.h"
using namespace QtDataSync;

AccessMessage::AccessMessage() :
	RegisterBaseMessage()
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const AccessMessage &message)
{
	stream << (RegisterBaseMessage)message;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, AccessMessage &message)
{
	stream.startTransaction();
	stream >> (RegisterBaseMessage&)message;
	stream.commitTransaction();
	return stream;
}
