#ifndef QTDATASYNC_USERDATANETWORKEXCHANGE_P_H
#define QTDATASYNC_USERDATANETWORKEXCHANGE_P_H

#include "qtdatasync_global.h"
#include "userdatanetworkexchange.h"
#include "authenticator.h"
#include "logger.h"

#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSettings>
#include <QtCore/qhash.h>

#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QNetworkDatagram>

#include <QtJsonSerializer/QJsonSerializer>

namespace QtDataSync {

class Q_DATASYNC_EXPORT ExchangeDatagram
{
	Q_GADGET

	Q_PROPERTY(DatagramType type MEMBER type)
	Q_PROPERTY(QString data MEMBER data)
	Q_PROPERTY(QString salt MEMBER salt)

public:
	enum DatagramType {
		Invalid,
		UserInfo,
		UserData
	};
	Q_ENUM(DatagramType)

	ExchangeDatagram();
	ExchangeDatagram(const QString &name);
	ExchangeDatagram(const QByteArray &data, const QByteArray &salt);

	DatagramType type;
	QString data;
	QString salt;
};

class Q_DATASYNC_EXPORT UserInfoData : public QSharedData
{
public:
	UserInfoData(const QString &name = {}, const QNetworkDatagram &datagram = {});
	UserInfoData(const UserInfoData &other);

	static QNetworkDatagram getDatagram(const UserInfo &userInfo);

	QString name;
	QNetworkDatagram datagram;
};

class Q_DATASYNC_EXPORT UserDataNetworkExchangePrivate
{
public:
	UserDataNetworkExchangePrivate(const QString &setupName, UserDataNetworkExchange *q_ptr);

	UserDataNetworkExchange *q;
	Authenticator *authenticator;
	QJsonSerializer *serializer;
	QUdpSocket *socket;
	QSettings *settings;
	QTimer *timer;

	QList<UserInfo> users;
	QList<int> timeouts;
	QHash<UserInfo, ExchangeDatagram> messageCache;

	Logger *logger;

	int findInfo(const QNetworkDatagram &datagram) const;

	//void sendData(const UserInfo &user, const QString &key);
	void receiveUserInfo(const ExchangeDatagram &message, const QNetworkDatagram &datagram);
	void receiveUserData(const ExchangeDatagram &message, const QNetworkDatagram &datagram);
};

}

#endif // QTDATASYNC_USERDATANETWORKEXCHANGE_P_H
