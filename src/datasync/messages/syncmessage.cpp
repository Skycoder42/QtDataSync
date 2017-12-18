#include "syncmessage_p.h"

#include <QtCore/QMetaEnum>

using namespace QtDataSync;

SyncMessage::SyncMessage() {}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const SyncMessage &message)
{
	Q_UNUSED(message)
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, SyncMessage &message)
{
	Q_UNUSED(message)
	return stream;
}
