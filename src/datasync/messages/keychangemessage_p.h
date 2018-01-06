#ifndef KEYCHANGEMESSAGE_P_H
#define KEYCHANGEMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT KeyChangeMessage
{
	Q_GADGET

	Q_PROPERTY(quint32 nextIndex MEMBER nextIndex)

public:
	KeyChangeMessage(quint32 nextIndex = 0);

	quint32 nextIndex;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const KeyChangeMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, KeyChangeMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::KeyChangeMessage)

#endif // KEYCHANGEMESSAGE_P_H
