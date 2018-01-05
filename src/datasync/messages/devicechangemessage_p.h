#ifndef DEVICECHANGEMESSAGE_P_H
#define DEVICECHANGEMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"
#include "changemessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT DeviceChangeMessage : public ChangeMessage
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	DeviceChangeMessage(const QByteArray &dataId = {}, const QUuid &deviceId = {});

	QUuid deviceId;
};

class Q_DATASYNC_EXPORT DeviceChangeAckMessage : public ChangeAckMessage
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	DeviceChangeAckMessage(const DeviceChangeMessage &message = {});

	QUuid deviceId;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const DeviceChangeMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, DeviceChangeMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const DeviceChangeAckMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, DeviceChangeAckMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::DeviceChangeMessage)
Q_DECLARE_METATYPE(QtDataSync::DeviceChangeAckMessage)

#endif // DEVICECHANGEMESSAGE_P_H
