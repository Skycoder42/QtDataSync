#ifndef QTDATASYNC_USEREXCHANGEMANAGER_P_H
#define QTDATASYNC_USEREXCHANGEMANAGER_P_H

#include <QtCore/QTimer>
#include <QtCore/QPointer>

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
	UserInfoPrivate(QString name = {}, const QNetworkDatagram &datagram = {});
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

	AccountManager *manager = nullptr;
	QPointer<QUdpSocket> socket;
	QTimer *timer;

	bool allowReuseAddress = false;
	QHash<UserInfo, quint8> devices;
	QHash<UserInfo, QByteArray> exchangeData;

	void clearSocket();
};

}

#endif // QTDATASYNC_USEREXCHANGEMANAGER_P_H
