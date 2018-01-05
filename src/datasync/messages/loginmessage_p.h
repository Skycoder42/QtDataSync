#ifndef LOGINMESSAGE_P_H
#define LOGINMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"
#include "identifymessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT LoginMessage : public InitMessage
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)
	Q_PROPERTY(Utf8String name MEMBER name)

public:
	LoginMessage(const QUuid &deviceId = {}, const QString &name = {}, const QByteArray &nonce = {});

	QUuid deviceId;
	Utf8String name;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const LoginMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, LoginMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::LoginMessage)

#endif // LOGINMESSAGE_P_H
