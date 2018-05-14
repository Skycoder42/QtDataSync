#ifndef QTDATASYNC_MACUPDATEMESSAGE_P_H
#define QTDATASYNC_MACUPDATEMESSAGE_P_H

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT MacUpdateMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)
	Q_PROPERTY(QByteArray cmac MEMBER cmac)

public:
	MacUpdateMessage(quint32 keyIndex = 0, const QByteArray &cmac = {});

	quint32 keyIndex;
	QByteArray cmac;

protected:
	const QMetaObject *getMetaObject() const override;
};

class Q_DATASYNC_EXPORT MacUpdateAckMessage : public Message
{
	Q_GADGET

public:
	MacUpdateAckMessage();

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::MacUpdateMessage)
Q_DECLARE_METATYPE(QtDataSync::MacUpdateAckMessage)

#endif // QTDATASYNC_MACUPDATEMESSAGE_P_H
