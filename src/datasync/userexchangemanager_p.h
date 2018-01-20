#ifndef USEREXCHANGEMANAGER_P_H
#define USEREXCHANGEMANAGER_P_H

#include <QtCore/QTimer>

#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QNetworkDatagram>

#include "qtdatasync_global.h"
#include "userexchangemanager.h"
#include "accountmanager.h"
#include "logger.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT UserInfoPrivate : public QSharedData
{
public:
	UserInfoPrivate(const QString &name = {}, const QNetworkDatagram &datagram = {});
	UserInfoPrivate(const UserInfoPrivate &other);

	QString name;
	QHostAddress address;
	quint16 port;
};

class Q_DATASYNC_EXPORT UserExchangeManagerPrivate
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

#endif // USEREXCHANGEMANAGER_P_H
