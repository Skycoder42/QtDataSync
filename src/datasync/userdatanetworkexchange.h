#ifndef QTDATASYNC_USERDATANETWORKEXCHANGE_H
#define QTDATASYNC_USERDATANETWORKEXCHANGE_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/task.h"

#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>

class QHostAddress;

namespace QtDataSync {

class ExchangeDatagram;
class UserInfoData;
class Q_DATASYNC_EXPORT UserInfo
{
	Q_GADGET
	friend class UserInfoData;

	Q_PROPERTY(QString name READ name CONSTANT)
	Q_PROPERTY(QHostAddress address READ address CONSTANT)
	Q_PROPERTY(quint16 port READ port CONSTANT)

public:
	UserInfo();
	UserInfo(const UserInfo &other);
	UserInfo(UserInfoData *data);
	~UserInfo();

	QString name() const;
	QHostAddress address() const;
	quint16 port() const;

	bool operator==(const UserInfo &other) const;

private:
	QSharedDataPointer<UserInfoData> d;
};

Q_DATASYNC_EXPORT uint qHash(const QtDataSync::UserInfo &info, uint seed = 0);

class UserDataNetworkExchangePrivate;
class Q_DATASYNC_EXPORT UserDataNetworkExchange : public QObject
{
	Q_OBJECT
	friend class UserDataNetworkExchangePrivate;

	Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName RESET resetDeviceName NOTIFY deviceNameChanged)

	Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
	Q_PROPERTY(QList<UserInfo> users READ users NOTIFY usersChanged)

public:
	static const quint16 DataExchangePort;

	explicit UserDataNetworkExchange(QObject *parent = nullptr);
	explicit UserDataNetworkExchange(const QString &setupName, QObject *parent = nullptr);
	~UserDataNetworkExchange();

	quint16 port() const;
	QString deviceName() const;
	bool isActive() const;
	QList<UserInfo> users() const;

	QString socketError() const;

public Q_SLOTS:
	bool startExchange(quint16 port = DataExchangePort);
	bool startExchange(const QHostAddress &listenAddress, quint16 port = DataExchangePort);
	void stopExchange();

	void exportTo(const UserInfo &userInfo, const QString &password = {});
	GenericTask<void> importFrom(const UserInfo &userInfo, const QString &password = {});

	void setDeviceName(QString deviceName);
	void resetDeviceName();

Q_SIGNALS:
	void userDataReceived(const UserInfo &userInfo, bool secured);

	void deviceNameChanged(QString deviceName);
	void activeChanged(bool active);
	void usersChanged(QList<UserInfo> users);

private Q_SLOTS:
	void timeout();
	void newData();

private:
	QScopedPointer<UserDataNetworkExchangePrivate> d;
};

}

#endif // QTDATASYNC_USERDATANETWORKEXCHANGE_H
