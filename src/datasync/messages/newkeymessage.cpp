#include "newkeymessage_p.h"
using namespace QtDataSync;

NewKeyMessage::NewKeyMessage(const QUuid &deviceId) :
	deviceId(deviceId),
	keyIndex(0),
	scheme(),
	key(),
	cmac()
{}

QByteArray NewKeyMessage::signatureData() const
{
	return QByteArray::number(keyIndex) +
			scheme +
			key;
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const NewKeyMessage &message)
{
	stream << message.deviceId
		   << message.keyIndex
		   << message.scheme
		   << message.key
		   << message.cmac;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, NewKeyMessage &message)
{
	stream.startTransaction();
	stream >> message.deviceId
		   >> message.keyIndex
		   >> message.scheme
		   >> message.key
		   >> message.cmac;
	stream.commitTransaction();
	return stream;
}
