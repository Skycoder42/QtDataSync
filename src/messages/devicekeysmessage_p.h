#ifndef QTDATASYNC_DEVICEKEYSMESSAGE_P_H
#define QTDATASYNC_DEVICEKEYSMESSAGE_P_H

#include <QtCore/QUuid>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT DeviceKeysMessage : public Message
{
	Q_GADGET

	Q_PROPERTY(quint32 keyIndex MEMBER keyIndex)
	Q_PROPERTY(bool duplicated MEMBER duplicated)
	Q_PROPERTY(QList<QtDataSync::DeviceKeysMessage::DeviceKey> devices MEMBER devices)

public:
	typedef std::tuple<QUuid, QByteArray, QByteArray, QByteArray> DeviceKey; // (deviceid, scheme, key, mac)

	DeviceKeysMessage(quint32 keyIndex = 0);
	DeviceKeysMessage(quint32 keyIndex, const QList<DeviceKey> &devices);

	quint32 keyIndex;
	bool duplicated;
	QList<DeviceKey> devices;

protected:
	const QMetaObject *getMetaObject() const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::DeviceKeysMessage)
Q_DECLARE_METATYPE(QtDataSync::DeviceKeysMessage::DeviceKey)

#endif // QTDATASYNC_DEVICEKEYSMESSAGE_P_H
