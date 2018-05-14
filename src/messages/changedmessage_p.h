#ifndef QTDATASYNC_CHANGEDMESSAGE_P_H
#define QTDATASYNC_CHANGEDMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ChangedMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(quint64 dataIndex MEMBER dataIndex)
	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)
	Q_PROPERTY(QByteArray salt MEMBER salt)
	Q_PROPERTY(QByteArray data MEMBER data)

public:
	ChangedMessage();

	quint64 dataIndex;
	quint32 keyIndex;
	QByteArray salt;
	QByteArray data;

protected:
	const QMetaObject *getMetaObject() const override;
};

class Q_DATASYNC_EXPORT ChangedInfoMessage : public ChangedMessage
{
	Q_GADGET

	Q_PROPERTY(quint32 changeEstimate MEMBER changeEstimate)

public:
	ChangedInfoMessage(quint32 changeEstimate = 0);

	quint32 changeEstimate;

protected:
	const QMetaObject *getMetaObject() const override;
};

class Q_DATASYNC_EXPORT LastChangedMessage : public Message
{
	Q_GADGET

public:
	LastChangedMessage();

protected:
	const QMetaObject *getMetaObject() const override;
};

class Q_DATASYNC_EXPORT ChangedAckMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(quint64 dataIndex MEMBER dataIndex)

public:
	ChangedAckMessage(quint64 dataIndex = 0);

	quint64 dataIndex;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::ChangedMessage)
Q_DECLARE_METATYPE(QtDataSync::ChangedInfoMessage)
Q_DECLARE_METATYPE(QtDataSync::LastChangedMessage)
Q_DECLARE_METATYPE(QtDataSync::ChangedAckMessage)

#endif // QTDATASYNC_CHANGEDMESSAGE_P_H
