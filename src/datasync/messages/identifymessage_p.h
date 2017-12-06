#ifndef IDENTIFYMESSAGE_H
#define IDENTIFYMESSAGE_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT IdentifyMessage
{
	Q_GADGET

	Q_PROPERTY(quint32 nonce MEMBER nonce)

public:
	IdentifyMessage(quint32 nonce = 0);

	static IdentifyMessage createRandom();

	quint32 nonce;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const IdentifyMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, IdentifyMessage &message);
Q_DATASYNC_EXPORT QDebug operator<<(QDebug debug, const IdentifyMessage &message);

}

#endif // IDENTIFYMESSAGE_H
