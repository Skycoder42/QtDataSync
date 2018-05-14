#ifndef QTDATASYNC_NEWKEYMESSAGE_P_H
#define QTDATASYNC_NEWKEYMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"
#include "macupdatemessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT NewKeyMessage : public MacUpdateMessage
{
	Q_GADGET

	Q_PROPERTY(QByteArray scheme MEMBER scheme)
	Q_PROPERTY(QList<QtDataSync::NewKeyMessage::KeyUpdate> deviceKeys MEMBER deviceKeys)

public:
	typedef std::tuple<QUuid, QByteArray, QByteArray> KeyUpdate; //(deviceId, key, cmac)

	NewKeyMessage();

	QByteArray scheme;
	QList<KeyUpdate> deviceKeys;

	QByteArray signatureData(const KeyUpdate &deviceInfo) const;

protected:
	const QMetaObject *getMetaObject() const override;
};

class Q_DATASYNC_EXPORT NewKeyAckMessage : public MacUpdateAckMessage
{
	Q_GADGET

	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)

public:
	NewKeyAckMessage(const NewKeyMessage &message = {});

	quint32 keyIndex;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::NewKeyMessage)
Q_DECLARE_METATYPE(QtDataSync::NewKeyMessage::KeyUpdate)
Q_DECLARE_METATYPE(QtDataSync::NewKeyAckMessage)

#endif // QTDATASYNC_NEWKEYMESSAGE_P_H
