#include "keychangemessage_p.h"
using namespace QtDataSync;

KeyChangeMessage::KeyChangeMessage(quint32 nextIndex) :
	nextIndex(nextIndex)
{}

const QMetaObject *KeyChangeMessage::getMetaObject() const
{
	return &staticMetaObject;
}
