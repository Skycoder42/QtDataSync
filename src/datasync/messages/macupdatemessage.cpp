#include "macupdatemessage_p.h"
using namespace QtDataSync;

MacUpdateMessage::MacUpdateMessage(quint32 keyIndex, const QByteArray &cmac) :
	keyIndex(keyIndex),
	cmac(cmac)
{}

const QMetaObject *MacUpdateMessage::getMetaObject() const
{
	return &staticMetaObject;
}



MacUpdateAckMessage::MacUpdateAckMessage() {}

const QMetaObject *MacUpdateAckMessage::getMetaObject() const
{
	return &staticMetaObject;
}
