#include "donemessage_p.h"
using namespace QtDataSync;

DoneMessage::DoneMessage(const QByteArray &dataId) :
	dataId(dataId)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const DoneMessage &message)
{
	stream << message.dataId;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, DoneMessage &message)
{
	stream.startTransaction();
	stream >> message.dataId;
	stream.commitTransaction();
	return stream;
}
