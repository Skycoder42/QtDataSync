#ifndef QTDATASYNC_CHANGEMESSAGE_P_H
#define QTDATASYNC_CHANGEMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ChangeMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(QByteArray dataId MEMBER dataId)
	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)
	Q_PROPERTY(QByteArray salt MEMBER salt)
	Q_PROPERTY(QByteArray data MEMBER data)

public:
	ChangeMessage(const QByteArray &dataId = {});

	QByteArray dataId;
	quint32 keyIndex;
	QByteArray salt;
	QByteArray data;

protected:
	const QMetaObject *getMetaObject() const override;
};

class Q_DATASYNC_EXPORT ChangeAckMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(QByteArray dataId MEMBER dataId)

public:
	ChangeAckMessage(const ChangeMessage &message = {});

	QByteArray dataId;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::ChangeMessage)
Q_DECLARE_METATYPE(QtDataSync::ChangeAckMessage)

#endif // QTDATASYNC_CHANGEMESSAGE_P_H
