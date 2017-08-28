#include "userdatanetworkexchange.h"
#include "userdatanetworkexchange_p.h"
#include "setup_p.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDebug>
#include <QtNetwork/QNetworkInterface>

#include <qtinyaes.h>
#include <qrng.h>

using namespace QtDataSync;

#define QTDATASYNC_LOG d->logger

const quint16 UserDataNetworkExchange::DataExchangePort = 13742;

UserDataNetworkExchange::UserDataNetworkExchange(QObject *parent) :
	QObject(parent),
	d(new UserDataNetworkExchangePrivate(Setup::DefaultSetup, this))
{}

UserDataNetworkExchange::UserDataNetworkExchange(const QString &setupName, QObject *parent) :
	QObject(parent),
	d(new UserDataNetworkExchangePrivate(setupName, this))
{}

UserDataNetworkExchange::~UserDataNetworkExchange() {}

quint16 UserDataNetworkExchange::port() const
{
	return d->socket->localPort();
}

QString UserDataNetworkExchange::deviceName() const
{
	return d->settings->value(QStringLiteral("name"), QSysInfo::machineHostName()).toString();
}

bool UserDataNetworkExchange::isActive() const
{
	return d->timer->isActive();
}

QList<UserInfo> UserDataNetworkExchange::users() const
{
	return d->users;
}

QString UserDataNetworkExchange::socketError() const
{
	return d->socket->errorString();
}

bool UserDataNetworkExchange::startExchange(quint16 port, bool allowReuseAddress)
{
	return startExchange(QHostAddress::Any, port, allowReuseAddress);
}

bool UserDataNetworkExchange::startExchange(const QHostAddress &listenAddress, quint16 port, bool allowReuseAddress)
{
	d->users.clear();
	d->messageCache.clear();
	emit usersChanged(d->users);

	d->timer->stop();
	if(d->socket->isOpen())
		d->socket->close();

	if(!d->socket->bind(listenAddress,
						port,
						allowReuseAddress ?
							QAbstractSocket::ShareAddress :
							QAbstractSocket::DontShareAddress)) {
		return false;
	}

	d->timer->start();
	emit activeChanged(true);
	timeout();
	return true;
}

void UserDataNetworkExchange::stopExchange()
{
	d->timer->stop();
	if(d->socket->isOpen())
		d->socket->close();
	emit activeChanged(false);

	d->users.clear();
	d->messageCache.clear();
	emit usersChanged(d->users);
}

void UserDataNetworkExchange::exportTo(const UserInfo &userInfo, const QString &password)
{
	auto data = d->authenticator->exportUserData();
	QByteArray salt;

	if(!password.isNull()) {
		auto key = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha3_256);
		QRng rng;
		rng.setSecurityLevel(QRng::HighSecurity);
		salt = rng.generateRandom(QTinyAes::BLOCKSIZE);
		data = QTinyAes::cbcEncrypt(key, salt, data);
	}

	ExchangeDatagram message(data, salt);
	auto datagram = UserInfoData::getDatagram(userInfo).makeReply(d->serializer->serializeTo(message));
	//reset interface & sender (because of broadcast)
	datagram.setInterfaceIndex(0);
	datagram.setSender(QHostAddress());
	d->socket->writeDatagram(datagram);
}

GenericTask<void> UserDataNetworkExchange::importFrom(const UserInfo &userInfo, const QString &password)
{
	auto message = d->messageCache.take(userInfo);
	if(message.type != ExchangeDatagram::UserData) {
		QFutureInterface<QVariant> d;
		d.reportStarted();
		d.reportException(DataSyncException("No user data has been received for the given user"));
		d.reportFinished();
		return GenericTask<void>(d);
	}

	auto data = QByteArray::fromBase64(message.data.toUtf8());
	if(!message.salt.isEmpty()) {
		auto key = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha3_256);
		auto salt = QByteArray::fromBase64(message.salt.toUtf8());
		data = QTinyAes::cbcDecrypt(key, salt, data);
	}

	return d->authenticator->importUserData(data);
}

void UserDataNetworkExchange::setDeviceName(QString deviceName)
{
	if (this->deviceName() == deviceName)
		return;

	d->settings->setValue(QStringLiteral("name"), deviceName);
	emit deviceNameChanged(deviceName);

	if(d->timer->isActive()) {
		timeout();//send name immediatly
		d->timer->start();//and restart timer for normal delays
	}
}

void UserDataNetworkExchange::resetDeviceName()
{
	d->settings->remove(QStringLiteral("name"));
	emit deviceNameChanged(QSysInfo::machineHostName());
	if(d->timer->isActive()) {
		timeout();//send name immediatly
		d->timer->start();//and restart timer for normal delays
	}
}

void UserDataNetworkExchange::timeout()
{
	d->socket->writeDatagram(d->serializer->serializeTo<ExchangeDatagram>(deviceName()), QHostAddress::Broadcast, d->socket->localPort());
}

void UserDataNetworkExchange::newData()
{
	while(d->socket->hasPendingDatagrams()) {
		auto datagram = d->socket->receiveDatagram();
		if(!datagram.isValid())
			continue;

		//skip messages from myself
		auto isSelf = false;
		if(datagram.senderPort() == d->socket->localPort()) {
			foreach(auto addr, QNetworkInterface::allAddresses()) {
				if(addr.isEqual(datagram.senderAddress())) {
					isSelf = true;
					break;
				}
			}
		}
		if(isSelf)
			continue;

		try {
			auto message = d->serializer->deserializeFrom<ExchangeDatagram>(datagram.data());
			switch (message.type) {
			case ExchangeDatagram::Invalid: //ignored
				break;
			case ExchangeDatagram::UserInfo:
				d->receiveUserInfo(message, datagram);
				break;
			case ExchangeDatagram::UserData:
				d->receiveUserData(message, datagram);
				break;
			default:
				Q_UNREACHABLE();
				break;
			}
		} catch(QJsonDeserializationException &e) {
			logWarning() << "Received invalid datagram. Serializer error:" << e.what();
		}
	}
}

// ------------- Private Implementation -------------

UserDataNetworkExchangePrivate::UserDataNetworkExchangePrivate(const QString &setupName, UserDataNetworkExchange *q_ptr) :
	q(q_ptr),
	authenticator(Setup::authenticatorForSetup<Authenticator>(q, setupName)),
	serializer(new QJsonSerializer(q)),
	socket(new QUdpSocket(q)),
	settings(nullptr),
	timer(new QTimer(q)),
	users(),
	messageCache(),
	logger(nullptr)
{
	auto defaults = SetupPrivate::defaults(setupName);
	logger = defaults->createLogger("exchange", q);
	settings = defaults->createSettings(q);

	settings->beginGroup(QStringLiteral("NetworkExchange"));
	serializer->setEnumAsString(true);
	timer->setTimerType(Qt::VeryCoarseTimer);
	timer->setInterval(2000);

	QObject::connect(socket, &QUdpSocket::readyRead,
					 q, &UserDataNetworkExchange::newData);
	QObject::connect(timer, &QTimer::timeout,
					 q, &UserDataNetworkExchange::timeout);
}

int UserDataNetworkExchangePrivate::findInfo(const QNetworkDatagram &datagram) const
{
	UserInfo dummyInfo(new UserInfoData({}, datagram));
	return users.indexOf(dummyInfo);
}

void UserDataNetworkExchangePrivate::receiveUserInfo(const ExchangeDatagram &message, const QNetworkDatagram &datagram)
{
	auto name = message.data;
	if(name.isEmpty())
		name = UserDataNetworkExchange::tr("<Unnamed>");

	UserInfo userInfo(new UserInfoData(name, datagram));

	auto wasChanged = false;
	auto index = findInfo(datagram);
	if(index != -1) {
		if(users[index].name() != name) {
			users[index] = userInfo;
			wasChanged = true;
		}
	} else {
		users.append(userInfo);
		wasChanged = true;
	}

	if(wasChanged)
		emit q->usersChanged(users);
}

void UserDataNetworkExchangePrivate::receiveUserData(const ExchangeDatagram &message,  const QNetworkDatagram &datagram)
{
	auto index = findInfo(datagram);

	if(index == -1) {// create an unnamed user if no user was found
		UserInfo userInfo(new UserInfoData(UserDataNetworkExchange::tr("<Unnamed>"), datagram));
		users.append(userInfo);
		index = users.size() - 1;
	}

	auto userInfo = users[index];
	messageCache.insert(userInfo, message);
	emit q->userDataReceived(userInfo, !message.salt.isEmpty());
}

// ------------- UserInfo Implementation -------------

UserInfo::UserInfo() :
	d(new UserInfoData())
{}

UserInfo::UserInfo(const UserInfo &other) :
	d(other.d)
{}

UserInfo::UserInfo(UserInfoData *data) :
	d(data)
{}

UserInfo::~UserInfo() {}

QString UserInfo::name() const
{
	return d->name;
}

QHostAddress UserInfo::address() const
{
	return d->datagram.senderAddress();
}

quint16 UserInfo::port() const
{
	return (quint16)d->datagram.senderPort();
}

UserInfo &UserInfo::operator=(const UserInfo &other)
{
	d = other.d;
	return *this;
}

bool UserInfo::operator==(const UserInfo &other) const
{
	return address() == other.address() &&
			port() == other.port();
}

uint QtDataSync::qHash(const UserInfo &info, uint seed)
{
	return ::qHash(info.address(), seed) ^
			::qHash(info.port(), seed);
}

QDebug operator<<(QDebug stream, const UserInfo &userInfo)
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

// ------------- UserInfoData Implementation -------------

UserInfoData::UserInfoData(const QString &name, const QNetworkDatagram &datagram) :
	QSharedData(),
	name(name),
	datagram(datagram)
{}

UserInfoData::UserInfoData(const UserInfoData &other):
	QSharedData(other),
	name(other.name),
	datagram(other.datagram)
{}

QNetworkDatagram UserInfoData::getDatagram(const UserInfo &userInfo)
{
	return userInfo.d->datagram;
}

// ------------- ExchangeDatagram Implementation -------------

ExchangeDatagram::ExchangeDatagram() :
	type(Invalid),
	data(),
	salt()
{}

ExchangeDatagram::ExchangeDatagram(const QString &name) :
	type(UserInfo),
	data(name),
	salt()
{}

ExchangeDatagram::ExchangeDatagram(const QByteArray &data, const QByteArray &salt) :
	type(UserData),
	data(QString::fromUtf8(data.toBase64())),
	salt(salt.isEmpty() ? QString() : QString::fromUtf8(salt.toBase64()))
{}
