#ifndef QTDATASYNC_REMOVEMESSAGE_P_H
#define QTDATASYNC_REMOVEMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT RemoveMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	RemoveMessage(const QUuid &deviceId = {});

	QUuid deviceId;

protected:
	const QMetaObject *getMetaObject() const override;
};

class Q_DATASYNC_EXPORT RemoveAckMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	RemoveAckMessage(const QUuid &deviceId = {});

	QUuid deviceId;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::RemoveMessage)
Q_DECLARE_METATYPE(QtDataSync::RemoveAckMessage)

#endif // QTDATASYNC_REMOVEMESSAGE_P_H
