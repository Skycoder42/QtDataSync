#include "userexchangemanager.h"
#include "userexchangemanager_p.h"
using namespace QtDataSync;

const quint16 UserExchangeManager::DataExchangePort(13742);

UserExchangeManager::UserExchangeManager(QObject *parent) :
	UserExchangeManager(DefaultSetup, parent)
{}

UserExchangeManager::UserExchangeManager(const QString &setupName, QObject *parent) :
	UserExchangeManager(new AccountManager(setupName), parent)
{
	d->manager->setParent(this);
}

UserExchangeManager::UserExchangeManager(AccountManager *manager, QObject *parent) :
	QObject(parent),
	d(new UserExchangeManagerPrivate())
{}

UserExchangeManager::~UserExchangeManager() {}

AccountManager *UserExchangeManager::accountManager() const
{
	return d->manager;
}

quint16 UserExchangeManager::port() const
{
	return d->socket->localPort();
}

bool UserExchangeManager::isActive() const
{
	return d->socket->state() == QAbstractSocket::BoundState;
}

QList<UserInfo> UserExchangeManager::users() const
{
	return d->users;
}

QString UserExchangeManager::lastError() const
{
	return d->socket->errorString();
}

bool UserExchangeManager::startExchange(const QHostAddress &listenAddress, quint16 port, bool allowReuseAddress)
{
	d->users.clear();
	emit usersChanged(d->users);

	if(d->socket->isOpen())
		d->socket->close();

	if(!d->socket->bind(listenAddress,
						port,
						allowReuseAddress ?
							QAbstractSocket::ShareAddress :
							QAbstractSocket::DontShareAddress)) {
		return false;
	}

	emit activeChanged(true);
	return true;
}

void UserExchangeManager::stopExchange()
{
	if(d->socket->isOpen())
		d->socket->close();
	emit activeChanged(false);

	d->users.clear();
	emit usersChanged(d->users);
}

void UserExchangeManager::exportTo(const UserInfo &userInfo)
{

}

void UserExchangeManager::exportTrustedTo(const UserInfo &userInfo, const QString &password)
{

}

void UserExchangeManager::importFrom(const UserInfo &userInfo, const QString &password)
{

}

// ------------- UserInfo Implementation -------------

UserInfo::UserInfo() :
	d(new UserInfoPrivate())
{}

UserInfo::UserInfo(const UserInfo &other) :
	d(other.d)
{}

UserInfo::UserInfo(UserInfoPrivate *data) :
	d(data)
{}

UserInfo::~UserInfo() {}

QString UserInfo::name() const
{
	return d->name;
}

QHostAddress UserInfo::address() const
{
	return d->address;
}

quint16 UserInfo::port() const
{
	return d->port;
}

UserInfo &UserInfo::operator=(const UserInfo &other)
{
	d = other.d;
	return *this;
}

bool UserInfo::operator==(const UserInfo &other) const
{
	return d->address == other.d->address &&
			d->port == other.d->port;
}

uint QtDataSync::qHash(const UserInfo &info, uint seed)
{
	return ::qHash(info.address(), seed) ^
			::qHash(info.port(), seed);
}

QDebug QtDataSync::operator<<(QDebug stream, const UserInfo &userInfo)
{
	QDebugStateSaver saver(stream);
	stream.noquote().nospace() << "["
							   << userInfo.name() << "]("
							   << userInfo.address()
							   << ":"
							   << userInfo.port()
							   << ")";
	return stream;
}



UserInfoPrivate::UserInfoPrivate(const QString &name, const QHostAddress &address, const quint16 port) :
	QSharedData(),
	name(name),
	address(address),
	port(port)
{}

UserInfoPrivate::UserInfoPrivate(const UserInfoPrivate &other) :
	QSharedData(other),
	name(other.name),
	address(other.address),
	port(other.port)
{}
