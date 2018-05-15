#include "devicesmessage_p.h"
using namespace QtDataSync;

ListDevicesMessage::ListDevicesMessage() = default;

const QMetaObject *ListDevicesMessage::getMetaObject() const
{
	return &staticMetaObject;
}



DevicesMessage::DevicesMessage() = default;

const QMetaObject *DevicesMessage::getMetaObject() const
{
	return &staticMetaObject;
}
