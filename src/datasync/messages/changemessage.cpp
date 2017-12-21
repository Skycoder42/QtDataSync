#include "changemessage_p.h"
using namespace QtDataSync;

ChangeMessage::ChangeMessage(const QByteArray &dataId) :
	dataId(dataId),
	keyIndex(0),
	salt(),
	data()
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const ChangeMessage &message)
{
	stream << message.dataId
		   << message.keyIndex
		   << message.salt
		   << message.data;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, ChangeMessage &message)
{
	stream.startTransaction();
	stream >> message.dataId
		   >> message.keyIndex
		   >> message.salt
		   >> message.data;
	stream.commitTransaction();
	return stream;
}
