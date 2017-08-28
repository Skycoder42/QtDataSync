#ifndef QTDATASYNC_USERDATANETWORKEXCHANGE_H
#define QTDATASYNC_USERDATANETWORKEXCHANGE_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/task.h"

#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>

#include <QtNetwork/qhostaddress.h>

namespace QtDataSync {

class ExchangeDatagram;
class UserInfoData;
//! Provides information about a detected exchange user
class Q_DATASYNC_EXPORT UserInfo
{
	Q_GADGET
	friend class UserInfoData;

	//! The display name of the user
	Q_PROPERTY(QString name READ name CONSTANT)
	//! The ip address of the user
	Q_PROPERTY(QHostAddress address READ address CONSTANT)
	//! The port of the user
	Q_PROPERTY(quint16 port READ port CONSTANT)

public:
	//! Constructor
	UserInfo();
	//! Copy Constructor
	UserInfo(const UserInfo &other);
	//! Internal Constructor
	UserInfo(UserInfoData *data);
	//! Destructor
	~UserInfo();

	//! @readAcFn{UserInfo::name}
	QString name() const;
	//! @readAcFn{UserInfo::address}
	QHostAddress address() const;
	//! @readAcFn{UserInfo::port}
	quint16 port() const;

	//! Assignment operator
	UserInfo &operator=(const UserInfo &other);
	//! Equality operator
	bool operator==(const UserInfo &other) const;

private:
	QSharedDataPointer<UserInfoData> d;
};

//! qHash function overload for UserInfo, see qHash(int)
Q_DATASYNC_EXPORT uint qHash(const QtDataSync::UserInfo &info, uint seed = 0);

class UserDataNetworkExchangePrivate;
//! Class to exchange the user identity with another device
class Q_DATASYNC_EXPORT UserDataNetworkExchange : public QObject
{
	Q_OBJECT
	friend class UserDataNetworkExchangePrivate;

	//! The display name of this device
	Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName RESET resetDeviceName NOTIFY deviceNameChanged)

	//! Reports whether the instance is currently exchanging or not
	Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
	//! Holds all currently discovered users
	Q_PROPERTY(QList<QtDataSync::UserInfo> users READ users NOTIFY usersChanged)

public:
	//! The default port (13742) for the data exchange
	static const quint16 DataExchangePort;

	//! Constructor for the default setup
	explicit UserDataNetworkExchange(QObject *parent = nullptr);
	//! Constructor for the given setup
	explicit UserDataNetworkExchange(const QString &setupName, QObject *parent = nullptr);
	//! Destructor
	~UserDataNetworkExchange();

	//! Returns the currently used port
	quint16 port() const;
	//! @readAcFn{UserInfo::deviceName}
	QString deviceName() const;
	//! @readAcFn{UserInfo::active}
	bool isActive() const;
	//! @readAcFn{UserInfo::users}
	QList<UserInfo> users() const;

	//! Returns the last socket error
	QString socketError() const;

public Q_SLOTS:
	//! Start the exchange discovery on the specified port
	bool startExchange(quint16 port = DataExchangePort);
	//! Start the exchange discovery on the specified address and port
	bool startExchange(const QHostAddress &listenAddress, quint16 port = DataExchangePort, bool allowReuseAddress = false);
	//! Stops the exchange discovery
	void stopExchange();

	//! Sends the user identity of the setup to the given user
	void exportTo(const QtDataSync::UserInfo &userInfo, const QString &password = {});
	//! Imports the user identity, previously received by the given user
	GenericTask<void> importFrom(const QtDataSync::UserInfo &userInfo, const QString &password = {});

	//! @writeAcFn{UserInfo::deviceName}
	void setDeviceName(QString deviceName);
	//! @resetAcFn{UserInfo::deviceName}
	void resetDeviceName();

Q_SIGNALS:
	//! Is emitted, when a user identity was received from the given user
	void userDataReceived(const QtDataSync::UserInfo &userInfo, bool secured);

	//! @notifyAcFn{UserInfo::deviceName}
	void deviceNameChanged(QString deviceName);
	//! @notifyAcFn{UserInfo::active}
	void activeChanged(bool active);
	//! @notifyAcFn{UserInfo::users}
	void usersChanged(QList<QtDataSync::UserInfo> users);

private Q_SLOTS:
	void timeout();
	void newData();

private:
	QScopedPointer<UserDataNetworkExchangePrivate> d;
};

}

Q_DECLARE_METATYPE(QtDataSync::UserInfo)

//! Stream operator for QDebug
Q_DATASYNC_EXPORT QDebug operator<< (QDebug stream, const QtDataSync::UserInfo &userInfo);

#endif // QTDATASYNC_USERDATANETWORKEXCHANGE_H
