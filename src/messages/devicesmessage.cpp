#include "devicesmessage_p.h"
using namespace QtDataSync;

const QMetaObject *ListDevicesMessage::getMetaObject() const
{
	return &staticMetaObject;
}



const QMetaObject *DevicesMessage::getMetaObject() const
{
	return &staticMetaObject;
}
