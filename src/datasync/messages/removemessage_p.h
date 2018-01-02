#ifndef REMOVEMESSAGE_P_H
#define REMOVEMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT RemoveMessage
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	RemoveMessage(const QUuid &deviceId = {});

	QUuid deviceId;
};

class Q_DATASYNC_EXPORT RemovedMessage
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	RemovedMessage(const QUuid &deviceId = {});

	QUuid deviceId;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RemoveMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RemoveMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RemovedMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RemovedMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::RemoveMessage)
Q_DECLARE_METATYPE(QtDataSync::RemovedMessage)

#endif // REMOVEMESSAGE_P_H
