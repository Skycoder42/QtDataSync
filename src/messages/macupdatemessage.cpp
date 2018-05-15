#include "macupdatemessage_p.h"
using namespace QtDataSync;

MacUpdateMessage::MacUpdateMessage(quint32 keyIndex, QByteArray cmac) :
	keyIndex(keyIndex),
	cmac(std::move(cmac))
{}

const QMetaObject *MacUpdateMessage::getMetaObject() const
{
	return &staticMetaObject;
}



MacUpdateAckMessage::MacUpdateAckMessage() = default;

const QMetaObject *MacUpdateAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
