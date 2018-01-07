#ifndef WELCOMEMESSAGE_P_H
#define WELCOMEMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT WelcomeMessage
{
	Q_GADGET

	Q_PROPERTY(bool hasChanges MEMBER hasChanges)
	Q_PROPERTY(QList<KeyUpdate> keyUpdates MEMBER keyUpdates)

public:
	typedef std::tuple<quint32, QByteArray, QByteArray, QByteArray> KeyUpdate; //(keyIndex, scheme, key, cmac)

	WelcomeMessage(bool hasChanges = false, const QList<KeyUpdate> &keyUpdates = {});

	bool hasChanges;
	QList<KeyUpdate> keyUpdates;

	static QByteArray signatureData(const QUuid &deviceId, const KeyUpdate &deviceInfo);
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const WelcomeMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, WelcomeMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::WelcomeMessage)

#endif // WELCOMEMESSAGE_P_H
