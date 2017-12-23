#ifndef CHANGEDMESSAGE_P_H
#define CHANGEDMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ChangedMessage
{
	Q_GADGET

	Q_PROPERTY(quint64 dataIndex MEMBER dataIndex)
	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)
	Q_PROPERTY(QByteArray salt MEMBER salt)
	Q_PROPERTY(QByteArray data MEMBER data)

public:
	ChangedMessage();

	quint64 dataIndex;
	quint32 keyIndex;
	QByteArray salt;
	QByteArray data;
};

class Q_DATASYNC_EXPORT ChangedInfoMessage : public ChangedMessage
{
	Q_GADGET

	Q_PROPERTY(quint64 changeEstimate MEMBER changeEstimate)

public:
	ChangedInfoMessage();

	quint64 changeEstimate;
};

class Q_DATASYNC_EXPORT LastChangedMessage
{
	Q_GADGET

public:
	LastChangedMessage();
};

class Q_DATASYNC_EXPORT ChangedAckMessage
{
	Q_GADGET

	Q_PROPERTY(quint64 dataIndex MEMBER dataIndex)

public:
	ChangedAckMessage(quint64 dataIndex = 0);

	quint64 dataIndex;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const ChangedMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, ChangedMessage &message);

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const ChangedInfoMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, ChangedInfoMessage &message);

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const LastChangedMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, LastChangedMessage &message);

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const ChangedAckMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, ChangedAckMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::ChangedMessage)
Q_DECLARE_METATYPE(QtDataSync::ChangedInfoMessage)
Q_DECLARE_METATYPE(QtDataSync::LastChangedMessage)
Q_DECLARE_METATYPE(QtDataSync::ChangedAckMessage)

#endif // CHANGEDMESSAGE_P_H
