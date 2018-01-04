#include "changedmessage_p.h"
using namespace QtDataSync;

ChangedMessage::ChangedMessage() :
	dataIndex(0),
	keyIndex(0),
	salt(),
	data()
{}

ChangedInfoMessage::ChangedInfoMessage(quint32 changeEstimate) :
	ChangedMessage(),
	changeEstimate(changeEstimate)
{}

LastChangedMessage::LastChangedMessage() {}

ChangedAckMessage::ChangedAckMessage(quint64 dataIndex) :
	dataIndex(dataIndex)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const ChangedMessage &message)
{
	stream << message.dataIndex
		   << message.keyIndex
		   << message.salt
		   << message.data;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, ChangedMessage &message)
{
	stream.startTransaction();
	stream >> message.dataIndex
		   >> message.keyIndex
		   >> message.salt
		   >> message.data;
	stream.commitTransaction();
	return stream;
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const ChangedInfoMessage &message)
{
	stream << static_cast<ChangedMessage>(message)
		   << message.changeEstimate;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, ChangedInfoMessage &message)
{
	stream.startTransaction();
	stream >> static_cast<ChangedMessage&>(message)
		   >> message.changeEstimate;
	stream.commitTransaction();
	return stream;
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const LastChangedMessage &message)
{
	Q_UNUSED(message)
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, LastChangedMessage &message)
{
	Q_UNUSED(message)
	return stream;
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const ChangedAckMessage &message)
{
	stream << message.dataIndex;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, ChangedAckMessage &message)
{
	stream.startTransaction();
	stream >> message.dataIndex;
	stream.commitTransaction();
	return stream;
}
