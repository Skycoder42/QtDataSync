#include "syncmessage_p.h"

#include <QtCore/QMetaEnum>

using namespace QtDataSync;

SyncMessage::SyncMessage() = default;

const QMetaObject *SyncMessage::getMetaObject() const
{
	return &staticMetaObject;
}
