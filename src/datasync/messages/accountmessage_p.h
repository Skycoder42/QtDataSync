#ifndef ACCOUNTMESSAGE_P_H
#define ACCOUNTMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"

#include <cryptopp/rng.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT AccountMessage
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	AccountMessage(const QUuid &deviceId = {});

	QUuid deviceId;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const AccountMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, AccountMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::AccountMessage)

#endif // ACCOUNTMESSAGE_P_H
