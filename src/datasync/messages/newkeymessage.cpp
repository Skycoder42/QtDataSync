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

const QMetaObject *NewKeyMessage::getMetaObject() const
{
	return &staticMetaObject;
}



NewKeyAckMessage::NewKeyAckMessage(const NewKeyMessage &message) :
	MacUpdateAckMessage(),
	keyIndex(message.keyIndex)
{}

const QMetaObject *NewKeyAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
