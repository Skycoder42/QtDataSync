#ifndef QTDATASYNC_LOGINMESSAGE_P_H
#define QTDATASYNC_LOGINMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"
#include "identifymessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT LoginMessage : public InitMessage
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)
	Q_PROPERTY(QtDataSync::Utf8String deviceName MEMBER deviceName)

public:
	LoginMessage(const QUuid &deviceId = {}, const QString &deviceName = {}, const QByteArray &nonce = {});

	QUuid deviceId;
	Utf8String deviceName;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::LoginMessage)

#endif // QTDATASYNC_LOGINMESSAGE_P_H
