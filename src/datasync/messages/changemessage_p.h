#ifndef CHANGEMESSAGE_P_H
#define CHANGEMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ChangeMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray dataId MEMBER dataId)
	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)
	Q_PROPERTY(QByteArray salt MEMBER salt)
	Q_PROPERTY(QByteArray data MEMBER data)

public:
	ChangeMessage(const QByteArray &dataId = {});

	QByteArray dataId;
	quint32 keyIndex;
	QByteArray salt;
	QByteArray data;
};

class Q_DATASYNC_EXPORT ChangeAckMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray dataId MEMBER dataId)

public:
	ChangeAckMessage(const ChangeMessage &message = {});

	QByteArray dataId;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const ChangeMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, ChangeMessage &message);

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const ChangeAckMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, ChangeAckMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::ChangeMessage)
Q_DECLARE_METATYPE(QtDataSync::ChangeAckMessage)

#endif // CHANGEMESSAGE_P_H
