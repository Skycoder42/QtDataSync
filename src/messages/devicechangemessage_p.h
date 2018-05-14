#ifndef QTDATASYNC_DEVICECHANGEMESSAGE_P_H
#define QTDATASYNC_DEVICECHANGEMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"
#include "changemessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT DeviceChangeMessage : public ChangeMessage
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	DeviceChangeMessage(const QByteArray &dataId = {}, const QUuid &deviceId = {});

	QUuid deviceId;

protected:
	const QMetaObject *getMetaObject() const override;
};

class Q_DATASYNC_EXPORT DeviceChangeAckMessage : public ChangeAckMessage
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)

public:
	DeviceChangeAckMessage(const DeviceChangeMessage &message = {});

	QUuid deviceId;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::DeviceChangeMessage)
Q_DECLARE_METATYPE(QtDataSync::DeviceChangeAckMessage)

#endif // QTDATASYNC_DEVICECHANGEMESSAGE_P_H
