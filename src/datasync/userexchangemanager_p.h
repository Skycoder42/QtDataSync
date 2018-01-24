#ifndef QTDATASYNC_USEREXCHANGEMANAGER_P_H
#define QTDATASYNC_USEREXCHANGEMANAGER_P_H

#include <QtCore/QTimer>

#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QNetworkDatagram>

#include "qtdatasync_global.h"
#include "userexchangemanager.h"
#include "accountmanager.h"
#include "logger.h"

namespace QtDataSync {

//no export needed
class UserInfoPrivate : public QSharedData
{
public:
	UserInfoPrivate(const QString &name = {}, const QNetworkDatagram &datagram = {});
	UserInfoPrivate(const UserInfoPrivate &other);

	QString name;
	QHostAddress address;
	quint16 port;
};

//no export needed
class UserExchangeManagerPrivate
{
public:
	enum DatagramType {
		DeviceInfo,
		DeviceDataUntrusted,
		DeviceDataTrusted
	};

	UserExchangeManagerPrivate(UserExchangeManager *q_ptr);

	AccountManager *manager;
	QUdpSocket *socket;
	QTimer *timer;

	bool allowReuseAddress;
	QHash<UserInfo, quint8> devices;
	QHash<UserInfo, QByteArray> exchangeData;
};

}

#endif // QTDATASYNC_USEREXCHANGEMANAGER_P_H
