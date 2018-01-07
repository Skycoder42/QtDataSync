#include "newkeymessage_p.h"
using namespace QtDataSync;

NewKeyMessage::NewKeyMessage() :
	scheme(),
	deviceKeys()
{}

QByteArray NewKeyMessage::signatureData(const NewKeyMessage::KeyUpdate &deviceInfo) const
{
	// keyIndex, scheme, deviceId, key
	return QByteArray::number(keyIndex) +
			scheme +
			std::get<0>(deviceInfo).toRfc4122() +
			std::get<1>(deviceInfo);
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const NewKeyMessage &message)
{
	stream << static_cast<MacUpdateMessage>(message)
		   << message.scheme
		   << message.deviceKeys;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, NewKeyMessage &message)
{
	stream.startTransaction();
	stream >> static_cast<MacUpdateMessage&>(message)
		   >> message.scheme
		   >> message.deviceKeys;
	stream.commitTransaction();
	return stream;
}



NewKeyAckMessage::NewKeyAckMessage(const NewKeyMessage &message) :
	MacUpdateAckMessage(),
	keyIndex(message.keyIndex)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const NewKeyAckMessage &message)
{
	stream << static_cast<MacUpdateAckMessage>(message)
		   << message.keyIndex;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, NewKeyAckMessage &message)
{
	stream.startTransaction();
	stream >> static_cast<MacUpdateAckMessage&>(message)
		   >> message.keyIndex;
	stream.commitTransaction();
	return stream;
}
