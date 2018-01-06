#ifndef MACUPDATEMESSAGE_P_H
#define MACUPDATEMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT MacUpdateMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray cmac MEMBER cmac)

public:
	MacUpdateMessage(const QByteArray &cmac = {});

	QByteArray cmac;
};

class Q_DATASYNC_EXPORT MacUpdateAckMessage
{
	Q_GADGET

public:
	MacUpdateAckMessage();
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const MacUpdateMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, MacUpdateMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const MacUpdateAckMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, MacUpdateAckMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::MacUpdateMessage)
Q_DECLARE_METATYPE(QtDataSync::MacUpdateAckMessage)

#endif // MACUPDATEMESSAGE_P_H
