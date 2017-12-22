#ifndef DONEMESSAGE_P_H
#define DONEMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT DoneMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray dataId MEMBER dataId)

public:
	DoneMessage(const QByteArray &dataId = {});

	QByteArray dataId;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const DoneMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, DoneMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::DoneMessage)

#endif // DONEMESSAGE_P_H
