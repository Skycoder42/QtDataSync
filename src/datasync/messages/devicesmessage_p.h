#ifndef DEVICESMESSAGE_P_H
#define DEVICESMESSAGE_P_H

#include <tuple>

#include <QtCore/QUuid>
#include <QtCore/QList>

#include "message_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ListDevicesMessage
{
	Q_GADGET

public:
	ListDevicesMessage();
};

class Q_DATASYNC_EXPORT DevicesMessage
{
	Q_GADGET

	Q_PROPERTY(QList<DeviceInfo> devices MEMBER devices)

public:
	typedef std::tuple<QUuid, QString, QByteArray> DeviceInfo;

	DevicesMessage();

	QList<DeviceInfo> devices;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const ListDevicesMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, ListDevicesMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const DevicesMessage &message);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, DevicesMessage &message);

}

Q_DECLARE_METATYPE(QtDataSync::ListDevicesMessage)
Q_DECLARE_METATYPE(QtDataSync::DevicesMessage)

#endif // DEVICESMESSAGE_P_H
