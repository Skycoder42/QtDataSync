#ifndef USEREXCHANGEMANAGER_H
#define USEREXCHANGEMANAGER_H

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qshareddata.h>

#include <QtNetwork/qhostaddress.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

class AccountManager;

class UserInfoPrivate;
//! Provides information about a detected exchange user
class Q_DATASYNC_EXPORT UserInfo
{
	Q_GADGET
	friend class UserInfoPrivate;

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
	UserInfo(UserInfoPrivate *data);
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
	QSharedDataPointer<UserInfoPrivate> d;
};

class UserExchangeManagerPrivate;
class Q_DATASYNC_EXPORT UserExchangeManager : public QObject
{
	Q_OBJECT

	//! Reports whether the instance is currently exchanging or not
	Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
	//! Holds all currently discovered users
	Q_PROPERTY(QList<QtDataSync::UserInfo> users READ users NOTIFY usersChanged)

public:
	//! The default port (13742) for the data exchange
	static const quint16 DataExchangePort;

	explicit UserExchangeManager(QObject *parent = nullptr);
	explicit UserExchangeManager(const QString &setupName, QObject *parent = nullptr);
	explicit UserExchangeManager(AccountManager *manager, QObject *parent = nullptr);
	~UserExchangeManager();

	AccountManager *accountManager() const;

	//! Returns the currently used port
	quint16 port() const;
	//! @readAcFn{UserInfo::active}
	bool isActive() const;
	//! @readAcFn{UserInfo::users}
	QList<UserInfo> users() const;

	//! Returns the last socket error
	QString lastError() const;//TODO signal?

public Q_SLOTS:
	//! Start the exchange discovery on the specified port
	inline bool startExchange(quint16 port = DataExchangePort) {
		return startExchange(QHostAddress::Any, port);
	}
	//! Start the exchange discovery on the specified address and port
	bool startExchange(const QHostAddress &listenAddress, quint16 port = DataExchangePort, bool allowReuseAddress = false);
	//! Stops the exchange discovery
	void stopExchange();

	//! Sends the user identity of the setup to the given user
	void exportTo(const QtDataSync::UserInfo &userInfo);
	void exportTrustedTo(const QtDataSync::UserInfo &userInfo, const QString &password);
	//! Imports the user identity, previously received by the given user
	void importFrom(const QtDataSync::UserInfo &userInfo, const QString &password = {});

Q_SIGNALS:
	//! Is emitted, when a user identity was received from the given user
	void userDataReceived(const QtDataSync::UserInfo &userInfo, bool trusted);

	//! @notifyAcFn{UserInfo::active}
	void activeChanged(bool active);
	//! @notifyAcFn{UserInfo::users}
	void usersChanged(QList<QtDataSync::UserInfo> users);

private:
	QScopedPointer<UserExchangeManagerPrivate> d;
};

//! qHash function overload for UserInfo, see qHash(int)
Q_DATASYNC_EXPORT uint qHash(const QtDataSync::UserInfo &info, uint seed = 0);
//! Stream operator for QDebug
Q_DATASYNC_EXPORT QDebug operator<<(QDebug stream, const QtDataSync::UserInfo &userInfo);

}

Q_DECLARE_METATYPE(QtDataSync::UserInfo)

#endif // USEREXCHANGEMANAGER_H
