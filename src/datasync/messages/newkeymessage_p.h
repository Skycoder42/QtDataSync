#ifndef NEWKEYMESSAGE_P_H
#define NEWKEYMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT NewKeyMessage
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId MEMBER deviceId)
	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)
	Q_PROPERTY(QByteArray scheme MEMBER scheme)
	Q_PROPERTY(QByteArray key MEMBER key)
	Q_PROPERTY(QByteArray cmac MEMBER cmac)

public:
	NewKeyMessage(const QUuid &deviceId = {});

	QUuid deviceId;
	quint32 keyIndex;
	QByteArray scheme;
	QByteArray key;
	QByteArray cmac;

	QByteArray signatureData() const;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const NewKeyMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, NewKeyMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::NewKeyMessage)

#endif // NEWKEYMESSAGE_P_H
