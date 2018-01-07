#ifndef NEWKEYMESSAGE_P_H
#define NEWKEYMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"
#include "macupdatemessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT NewKeyMessage : public MacUpdateMessage
{
	Q_GADGET

	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)
	Q_PROPERTY(QByteArray scheme MEMBER scheme)
	Q_PROPERTY(QList<KeyUpdate> deviceKeys MEMBER deviceKeys)

public:
	typedef std::tuple<QUuid, QByteArray, QByteArray> KeyUpdate; //(deviceId, key, cmac)

	NewKeyMessage();

	quint32 keyIndex;
	QByteArray scheme;
	QList<KeyUpdate> deviceKeys;

	QByteArray signatureData(const KeyUpdate &deviceInfo) const;
};

class Q_DATASYNC_EXPORT NewKeyAckMessage : public MacUpdateAckMessage
{
	Q_GADGET

	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)

public:
	NewKeyAckMessage(const NewKeyMessage &message = {});

	quint32 keyIndex;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const NewKeyMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, NewKeyMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const NewKeyAckMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, NewKeyAckMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::NewKeyMessage)
Q_DECLARE_METATYPE(QtDataSync::NewKeyAckMessage)

#endif // NEWKEYMESSAGE_P_H
