#include "syncmessage_p.h"

#include <QtCore/QMetaEnum>

using namespace QtDataSync;

const QMetaObject *SyncMessage::getMetaObject() const
{
	return &staticMetaObject;
}
