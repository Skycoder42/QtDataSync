#ifndef QTDATASYNC_USEREXCHANGEMANAGER_H
#define QTDATASYNC_USEREXCHANGEMANAGER_H

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
	UserInfo();
	//! Copy Constructor
	UserInfo(const UserInfo &other);
	//! @private
	UserInfo(UserInfoPrivate *data);
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
//! A helper class to exchange the account data between devices on the local network
class Q_DATASYNC_EXPORT UserExchangeManager : public QObject
{
	Q_OBJECT

	//! Reports whether the instance is currently exchanging or not
	Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
	//! Holds all currently discovered devices
	Q_PROPERTY(QList<QtDataSync::UserInfo> devices READ devices NOTIFY devicesChanged)

public:
	//! The default port (13742) for the data exchange
	static const quint16 DataExchangePort;

	//! @copydoc AccountManager::AccountManager(QObject *)
	explicit UserExchangeManager(QObject *parent = nullptr);
	//! @copydoc AccountManager::AccountManager(const QString &, QObject *)
	explicit UserExchangeManager(const QString &setupName, QObject *parent = nullptr);
	//! Constructor with an account manager to use for account import and export
	explicit UserExchangeManager(AccountManager *manager, QObject *parent = nullptr);
	~UserExchangeManager();

	//! The internally used account manager used to export and import data
	AccountManager *accountManager() const;

	//! Returns the currently used port
	quint16 port() const;
	//! @readAcFn{UserInfo::active}
	bool isActive() const;
	//! @readAcFn{UserInfo::users}
	QList<UserInfo> devices() const;

	//! Sends the user identity to the given user as untrusted export
	Q_INVOKABLE void exportTo(const QtDataSync::UserInfo &userInfo, bool includeServer);
	//! Sends the user identity to the given user as trusted export
	Q_INVOKABLE void exportTrustedTo(const QtDataSync::UserInfo &userInfo, bool includeServer, const QString &password);
	//! Imports the untrusted user data previously received by the given user
	void importFrom(const QtDataSync::UserInfo &userInfo,
					const std::function<void(bool,QString)> &completedFn,
					bool keepData = false);
	//! Imports the trusted user data previously received by the given user
	void importTrustedFrom(const QtDataSync::UserInfo &userInfo,
						   const QString &password,
						   const std::function<void(bool,QString)> &completedFn,
						   bool keepData = false);

public Q_SLOTS:
	//! Start the exchange discovery on the specified port
	inline bool startExchange(quint16 port = DataExchangePort) {
		return startExchange(QHostAddress::Any, port);
	}
	//! Start the exchange discovery on the specified address and port
	bool startExchange(const QHostAddress &listenAddress, quint16 port = DataExchangePort);
	//! Stops the exchange discovery
	void stopExchange();

Q_SIGNALS:
	//! Is emitted, when a user identity was received from the given user
	void userDataReceived(const QtDataSync::UserInfo &userInfo, bool trusted, QPrivateSignal);
	//! Is emitted when an error occured trying to export or import
	void exchangeError(const QString &errorString, QPrivateSignal);

	//! @notifyAcFn{UserInfo::active}
	void activeChanged(bool active, QPrivateSignal);
	//! @notifyAcFn{UserInfo::users}
	void devicesChanged(QList<QtDataSync::UserInfo> devices, QPrivateSignal);

protected:
	//! @private
	UserExchangeManager(QObject *parent, void*);
	//! @private
	void initManager(AccountManager *manager);

private Q_SLOTS:
	void timeout();
	void readDatagram();

private:
	QScopedPointer<UserExchangeManagerPrivate> d;
};

//! Overload of qHash to use UserInfo with QHash
Q_DATASYNC_EXPORT uint qHash(const QtDataSync::UserInfo &info, uint seed = 0);
//! Stream operator to stream into a QDebug
Q_DATASYNC_EXPORT QDebug operator<<(QDebug stream, const QtDataSync::UserInfo &userInfo);

}

Q_DECLARE_METATYPE(QtDataSync::UserInfo)
Q_DECLARE_TYPEINFO(QtDataSync::UserInfo, Q_MOVABLE_TYPE);

#endif // QTDATASYNC_USEREXCHANGEMANAGER_H
