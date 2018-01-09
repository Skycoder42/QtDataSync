#include "devicesmessage_p.h"
using namespace QtDataSync;

ListDevicesMessage::ListDevicesMessage() {}

const QMetaObject *ListDevicesMessage::getMetaObject() const
{
	return &staticMetaObject;
}



DevicesMessage::DevicesMessage() :
	devices()
{}

const QMetaObject *DevicesMessage::getMetaObject() const
{
	return &staticMetaObject;
}
