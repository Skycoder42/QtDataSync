#ifndef DEVICEKEYSMESSAGE_P_H
#define DEVICEKEYSMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT DeviceKeysMessage
{
	Q_GADGET

	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)
	Q_PROPERTY(QList<DeviceKey> devices MEMBER devices)

public:
	typedef std::tuple<QUuid, QByteArray, QByteArray, QByteArray> DeviceKey; // (deviceid, scheme, key, mac)

	DeviceKeysMessage(quint32 keyIndex = 0, const QList<DeviceKey> &devices = {});

	quint32 keyIndex;
	QList<DeviceKey> devices;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const DeviceKeysMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, DeviceKeysMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::DeviceKeysMessage)

#endif // DEVICEKEYSMESSAGE_P_H
