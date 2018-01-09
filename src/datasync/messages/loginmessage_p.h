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
	Q_PROPERTY(QtDataSync::Utf8String name MEMBER name)

public:
	LoginMessage(const QUuid &deviceId = {}, const QString &name = {}, const QByteArray &nonce = {});

	QUuid deviceId;
	Utf8String name;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::LoginMessage)

#endif // LOGINMESSAGE_P_H
