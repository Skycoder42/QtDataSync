#ifndef USEREXCHANGEMANAGER_P_H
#define USEREXCHANGEMANAGER_P_H

#include <QtNetwork/qudpsocket.h>

#include "qtdatasync_global.h"
#include "userexchangemanager.h"
#include "accountmanager.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT UserInfoPrivate : public QSharedData
{
public:
	UserInfoPrivate(const QString &name = {},
					const QHostAddress &address = {},
					const quint16 port = 0);
	UserInfoPrivate(const UserInfoPrivate &other);

	QString name;
	QHostAddress address;
	quint16 port;
};

class Q_DATASYNC_EXPORT UserExchangeManagerPrivate
{
public:
	AccountManager *manager;
	QUdpSocket *socket;

	QList<UserInfo> users;
};

}

#endif // USEREXCHANGEMANAGER_P_H
