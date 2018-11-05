#include "userexchangemanager.h"
#include "userexchangemanager_p.h"
#include "message_p.h"

#include <QtCore/QPointer>

#include <QtNetwork/QNetworkInterface>

using namespace QtDataSync;
using namespace std::chrono;
using std::function;

#if QT_HAS_INCLUDE(<chrono>)
#define scdtime(x) x
#else
#define scdtime(x) duration_cast<milliseconds>(x).count()
#endif

const quint16 UserExchangeManager::DataExchangePort(13742);

UserExchangeManager::UserExchangeManager(QObject *parent) :
	UserExchangeManager{DefaultSetup, parent}
{}

UserExchangeManager::UserExchangeManager(const QString &setupName, QObject *parent) :
	QObject{parent},
	d{new UserExchangeManagerPrivate(this)}
{
	initManager(new AccountManager(setupName, this));
	d->manager->setParent(this);
}

UserExchangeManager::UserExchangeManager(AccountManager *manager, QObject *parent) :
	QObject{parent},
	d{new UserExchangeManagerPrivate(this)}
{
	initManager(manager);
}

UserExchangeManager::UserExchangeManager(QObject *parent, void *) :
	QObject{parent},
	d{new UserExchangeManagerPrivate(this)}
{} //No init

void UserExchangeManager::initManager(AccountManager *manager)
{
	d->manager = manager;
	connect(d->manager, &AccountManager::setupNameChanged,
			this, [=](const QString &setupName){
		emit setupNameChanged(setupName, {});
	});

	d->timer->setInterval(scdtime(seconds(2)));
	d->timer->setTimerType(Qt::VeryCoarseTimer);
	connect(d->timer, &QTimer::timeout,
			this, &UserExchangeManager::timeout);
}

UserExchangeManager::~UserExchangeManager() = default;

AccountManager *UserExchangeManager::accountManager() const
{
	return d->manager;
}

QString UserExchangeManager::setupName() const
{
	return d->manager->setupName();
}

quint16 UserExchangeManager::port() const
{
	return d->socket ? d->socket->localPort() : 0;
}

bool UserExchangeManager::isActive() const
{
	return d->socket ? d->socket->state() == QAbstractSocket::BoundState : false;
}

QList<UserInfo> UserExchangeManager::devices() const
{
	return d->devices.keys();
}

bool UserExchangeManager::startExchange(const QHostAddress &listenAddress, quint16 port)
{
	d->devices.clear();
	d->exchangeData.clear();
	d->timer->stop();
	emit devicesChanged(d->devices.keys(), {});

	d->clearSocket();

	d->socket = new QUdpSocket{this};
	connect(d->socket, &QUdpSocket::readyRead,
			this, &UserExchangeManager::readDatagram);
	connect(d->socket, QOverload<QAbstractSocket::SocketError>::of(&QUdpSocket::error),
			this, [this]() {
		emit exchangeError(d->socket->errorString(), {});
	});

	if(!d->socket->bind(listenAddress, port)) {
		d->socket->deleteLater();
		d->socket.clear();
		return false;
	}

	d->timer->start();
	emit activeChanged(true, {});
	timeout();
	return true;
}

void UserExchangeManager::stopExchange()
{
	d->timer->stop();

	d->clearSocket();
	emit activeChanged(false, {});

	d->devices.clear();
	d->exchangeData.clear();
	emit devicesChanged(d->devices.keys(), {});
}

void UserExchangeManager::exportTo(const UserInfo &userInfo, bool includeServer)
{
	if(!d->socket)
		return;

	QPointer<UserExchangeManager> thisPtr(this);
	d->manager->exportAccount(includeServer, [thisPtr, this, userInfo](const QByteArray &exportData) {
		if(!thisPtr)
			return;

		QByteArray datagram;
		QDataStream stream(&datagram, QIODevice::WriteOnly | QIODevice::Unbuffered);
		Message::setupStream(stream);
		stream << static_cast<qint32>(UserExchangeManagerPrivate::DeviceDataUntrusted)
			   << exportData;

		if(d->socket)
			d->socket->writeDatagram(datagram, userInfo.address(), userInfo.port());
		else
			emit exchangeError(tr("Cannot export data if UserExchangeManager is not in active state"), {});
	}, [thisPtr, this](const QString &error) {
		if(!thisPtr)
			return;

		emit exchangeError(error, {});
	});
}

void UserExchangeManager::exportTrustedTo(const UserInfo &userInfo, bool includeServer, const QString &password)
{
	if(!d->socket)
		return;

	QPointer<UserExchangeManager> thisPtr(this);
	d->manager->exportAccountTrusted(includeServer, password, [thisPtr, this, userInfo](const QByteArray &exportData) {
		if(!thisPtr)
			return;

		QByteArray datagram;
		QDataStream stream(&datagram, QIODevice::WriteOnly | QIODevice::Unbuffered);
		Message::setupStream(stream);
		stream << static_cast<qint32>(UserExchangeManagerPrivate::DeviceDataTrusted)
			   << exportData;

		if(d->socket)
			d->socket->writeDatagram(datagram, userInfo.address(), userInfo.port());
		else
			emit exchangeError(tr("Cannot export data if UserExchangeManager is not in active state"), {});
	}, [thisPtr, this](const QString &error) {
		if(!thisPtr)
			return;

		emit exchangeError(error, {});
	});
}

void UserExchangeManager::importFrom(const UserInfo &userInfo, const function<void(bool,QString)> &completedFn, bool keepData)
{
	auto data = d->exchangeData.take(userInfo);
	if(data.isNull())
		emit exchangeError(tr("No import data received from passed user."), {});
	else
		d->manager->importAccount(data, completedFn, keepData);
}

void UserExchangeManager::importTrustedFrom(const UserInfo &userInfo, const QString &password, const function<void(bool,QString)> &completedFn, bool keepData)
{
	auto data = d->exchangeData.take(userInfo);
	if(data.isNull())
		emit exchangeError(tr("No import data received from passed user."), {});
	else
		d->manager->importAccountTrusted(data, password, completedFn, keepData);
}

void UserExchangeManager::timeout()
{
	auto erased = false;
	for(auto it = d->devices.begin(); it != d->devices.end();) {
		if(*it >= 5) {
			d->exchangeData.remove(it.key());
			it = d->devices.erase(it);
			erased = true;
		} else {
			(*it)++;
			it++;
		}
	}
	if(erased)
		emit devicesChanged(d->devices.keys(), {});

	QByteArray datagram;
	QDataStream stream(&datagram, QIODevice::WriteOnly | QIODevice::Unbuffered);
	Message::setupStream(stream);
	stream << static_cast<qint32>(UserExchangeManagerPrivate::DeviceInfo)
		   << d->manager->deviceName();

	if(d->socket)
		d->socket->writeDatagram(datagram, QHostAddress::Broadcast, d->socket->localPort());
}

void UserExchangeManager::readDatagram()
{
	while(d->socket && d->socket->hasPendingDatagrams()) {
		auto datagram = d->socket->receiveDatagram();

		auto isSelf = false;
		if(datagram.senderPort() == d->socket->localPort()) {
			for(const auto &addr : QNetworkInterface::allAddresses()) { // clazy:exclude=range-loop
				if(addr.isEqual(datagram.senderAddress())) {
					isSelf = true;
					break;
				}
			}
		}
		if(isSelf)
			continue;

		QDataStream stream(datagram.data());
		Message::setupStream(stream);
		qint32 type;
		stream.startTransaction();
		stream >> type;

		auto isInfo = true;
		QString name;
		auto trusted = false;
		QByteArray data;
		switch (type) {
		case UserExchangeManagerPrivate::DeviceInfo:
			isInfo = true;
			stream >> name;
			break;
		case UserExchangeManagerPrivate::DeviceDataUntrusted:
			isInfo = false;
			trusted = false;
			stream >> data;
			break;
		case UserExchangeManagerPrivate::DeviceDataTrusted:
			isInfo = false;
			trusted = true;
			stream >> data;
			break;
		default:
			qWarning() << "UserExchangeManager: Skipped invalid message from"
					   << UserInfo(new UserInfoPrivate(name, datagram));
			continue; //jump to next loop iteration
		}

		if(stream.commitTransaction()) {
			if(name.isEmpty())
				name = tr("<Unnamed>");
			UserInfo info(new UserInfoPrivate(name, datagram));

			//try to find the already existing user info for that data
			bool found = false;
			for(const auto &key : d->devices.keys()) { // clazy:exclude=range-loop
				if(info == key) {
					found = true;
					if(isInfo) { //isInfo -> reset timeout, update name if neccessary
						d->devices.insert(info, 0);
						if(info.name() != key.name())
							emit devicesChanged(d->devices.keys(), {});
					} else //!isInfo -> get the entry name
						info = key;
					break;
				}
			}

			if(isInfo) {
				if(!found) {//if not found -> simpy add to list
					d->devices.insert(info, 0);
					emit devicesChanged(d->devices.keys(), {});
				}
			} else { //data -> store it and emit
				d->exchangeData.insert(info, data);
				emit userDataReceived(info, trusted, {});
			}
		} else {
			emit exchangeError(tr("Invalid data received from %1:%2.")
							   .arg(datagram.senderAddress().toString())
							   .arg(datagram.senderPort()), {});
		}
	}
}



UserExchangeManagerPrivate::UserExchangeManagerPrivate(UserExchangeManager *q_ptr) :
	timer{new QTimer(q_ptr)}
{}

void UserExchangeManagerPrivate::clearSocket()
{
	if(socket) {
		if(socket->isOpen())
			socket->close();
		socket->deleteLater();
		socket.clear();
	}
}


// ------------- UserInfo Implementation -------------

UserInfo::UserInfo() :
	d{new UserInfoPrivate()}
{}

UserInfo::UserInfo(const UserInfo &other) = default;

UserInfo::UserInfo(UserInfo &&other) noexcept = default;

UserInfo::UserInfo(UserInfoPrivate *data) :
	d(data)
{}

UserInfo::~UserInfo() = default;

UserInfo &UserInfo::operator=(const UserInfo &other) = default;

UserInfo &UserInfo::operator=(UserInfo &&other) noexcept = default;

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

bool UserInfo::operator==(const UserInfo &other) const
{
	return d == other.d || (
		d->address == other.d->address &&
		d->port == other.d->port);
}

bool UserInfo::operator!=(const UserInfo &other) const
{
	return d != other.d && (
		d->address != other.d->address ||
		d->port != other.d->port);
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



UserInfoPrivate::UserInfoPrivate(QString name, const QNetworkDatagram &datagram) :
	QSharedData{},
	name{std::move(name)},
	address{datagram.senderAddress()},
	port{static_cast<quint16>(datagram.senderPort())}
{}

UserInfoPrivate::UserInfoPrivate(const UserInfoPrivate &other) = default;
