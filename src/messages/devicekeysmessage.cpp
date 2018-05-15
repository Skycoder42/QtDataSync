#include "devicekeysmessage_p.h"
using namespace QtDataSync;

DeviceKeysMessage::DeviceKeysMessage(quint32 keyIndex) :
	keyIndex(keyIndex),
	duplicated(true)
{}

DeviceKeysMessage::DeviceKeysMessage(quint32 keyIndex, QList<DeviceKey> devices) :
	keyIndex(keyIndex),
	duplicated(false),
	devices(std::move(devices))
{}

const QMetaObject *DeviceKeysMessage::getMetaObject() const
{
	return &staticMetaObject;
}
