#include "client.h"
#include "app.h"
#include "identifymessage_p.h"
#include "accountmessage_p.h"
#include "welcomemessage_p.h"

#include <chrono>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUuid>

#include <QtConcurrent/QtConcurrentRun>

using namespace QtDataSync;

#undef qDebug
#define qDebug(...) qCDebug(logFn, __VA_ARGS__)
#undef qInfo
#define qInfo(...) qCInfo(logFn, __VA_ARGS__)
#undef qWarning
#define qWarning(...) qCWarning(logFn, __VA_ARGS__)
#undef qCritical
#define qCritical(...) qCCritical(logFn, __VA_ARGS__)

#define LOCK QMutexLocker _(&_lock)

const QByteArray Client::PingMessage(1, 0xFF);
QThreadStorage<CryptoPP::AutoSeededRandomPool> Client::rngPool;

Client::Client(DatabaseController *database, QWebSocket *websocket, QObject *parent) :
	QObject(parent),
	_catStr(),
	_logCat(new QLoggingCategory("client.unkown")),
	_database(database),
	_socket(websocket),
	_deviceId(),
	_idleTimer(nullptr),
	_runCount(0),
	_lock(),
	_state(Authenticating),
	_properties()
{
	_socket->setParent(this);

	connect(_socket, &QWebSocket::disconnected,
			this, &Client::closeClient);
	connect(_socket, &QWebSocket::binaryMessageReceived,
			this, &Client::binaryMessageReceived);
	connect(_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
			this, &Client::error);
	connect(_socket, &QWebSocket::sslErrors,
			this, &Client::sslErrors);

	auto idleTimeout = qApp->configuration()->value(QStringLiteral("server/idleTimeout"), 5).toInt();
	if(idleTimeout > 0) {
		_idleTimer = new QTimer(this);
		_idleTimer->setInterval(std::chrono::minutes(idleTimeout));
		_idleTimer->setSingleShot(true);
		connect(_idleTimer, &QTimer::timeout,
				this, &Client::timeout);
		_idleTimer->start();
	}

	run([this]() {
		LOCK;
		//initialize connection by sending indent message
		auto msg = IdentifyMessage::createRandom(rngPool.localData());
		_properties.insert("nonce", msg.nonce);
		sendMessage(serializeMessage(msg));
	});
}

QUuid Client::deviceId() const
{
	return _deviceId;
}

void Client::notifyChanged(const QString &type, const QString &key, bool changed)
{
	Q_UNIMPLEMENTED();
}

void Client::binaryMessageReceived(const QByteArray &message)
{
	if(message == PingMessage) {
		if(_idleTimer)
			_idleTimer->start();
		_socket->sendBinaryMessage(PingMessage);
		return;
	}

	run([message, this]() {
		QDataStream stream(message);
		setupStream(stream);
		stream.startTransaction();
		QByteArray name;
		stream >> name;
		if(!stream.commitTransaction())
			throw DataStreamException(stream);

		if(isType<RegisterMessage>(name)) {
			auto msg = deserializeMessage<RegisterMessage>(stream);
			onRegister(msg, stream);
		} else if(isType<LoginMessage>(name)) {
			auto msg = deserializeMessage<LoginMessage>(stream);
			onLogin(msg, stream);
		} else {
			qWarning() << "Unknown message received: " << message;
			close();
		}
	});
}

void Client::error()
{
	qWarning() << "Socket error:"
			   << _socket->errorString();
	if(_socket->state() == QAbstractSocket::ConnectedState)
		_socket->close();
}

void Client::sslErrors(const QList<QSslError> &errors)
{
	foreach(auto error, errors) {
		qWarning() << "SSL error:"
				   << error.errorString();
	}
	if(_socket->state() == QAbstractSocket::ConnectedState)
		_socket->close();
}

void Client::closeClient()
{
	if(_runCount == 0) {//save close -> delete only if no parallel stuff anymore (check every 500 ms)
		qDebug() << "Client disconnected";
		deleteLater();
	} else {
		auto destroyTimer = new QTimer(this);
		connect(destroyTimer, &QTimer::timeout, this, [this](){
			if(_runCount == 0) {
				qDebug() << "Client disconnected";
				deleteLater();
			}
		});
		destroyTimer->start(500);
	}
}

void Client::timeout()
{
	qDebug() << "Disconnecting idle client";
	_socket->close();
}

void Client::run(const std::function<void ()> &fn)
{
	_runCount++;
	QtConcurrent::run(qApp->threadPool(), [fn, this]() {
		try {
			fn();
			_runCount--;
		} catch (DatabaseException &e) {
			qWarning().noquote() << "Internal database error:" << e.errorString() //TODO print query?
								 << "\nResettings connection in hopes to fix it.";
			close();
			_runCount--;
		} catch (std::exception &e) {
			qWarning() << "Message error:" << e.what();
			close();
			_runCount--;
		} catch(...) {
			_runCount--;
			throw;
		}
	});
}

const QLoggingCategory &Client::logFn() const
{
	return (*(_logCat.data()));
}

void Client::close()
{
	QMetaObject::invokeMethod(_socket, "close", Qt::QueuedConnection);
}

void Client::sendMessage(const QByteArray &message)
{
	QMetaObject::invokeMethod(this, "doSend", Qt::QueuedConnection,
							  Q_ARG(QByteArray, message));
}

void Client::doSend(const QByteArray &message)
{
	_socket->sendBinaryMessage(message);
}

void Client::onRegister(const RegisterMessage &message, QDataStream &stream)
{
	LOCK;
	if(_state != Authenticating)
		throw ClientException("Received RegisterMessage in invalid state");

	QScopedPointer<AsymmetricCryptoInfo> crypto(message.createCryptoInfo(rngPool.localData()));
	verifySignature(stream, crypto->signatureKey(), crypto.data());
	if(_properties.take("nonce").toByteArray() != message.nonce)
		throw ClientException("Invalid nonce in RegisterMessagee");

	_deviceId = _database->addNewDevice(message.deviceName,
										message.signAlgorithm,
										message.signKey,
										message.cryptAlgorithm,
										message.cryptKey);
	_catStr = "client." + _deviceId.toByteArray();
	_logCat.reset(new QLoggingCategory(_catStr.constData()));

	qDebug() << "Created new device and user accounts";
	sendMessage(serializeMessage<AccountMessage>(_deviceId));
	_state = Idle;
}

void Client::onLogin(const LoginMessage &message, QDataStream &stream)
{
	LOCK;
	if(_state != Authenticating)
		throw ClientException("Received LoginMessage in invalid state");

	//load public key to verify signature
	QString name;
	QScopedPointer<AsymmetricCryptoInfo> crypto(_database->loadCrypto(message.deviceId, rngPool.localData(), name));
	verifySignature(stream, crypto->signatureKey(), crypto.data());
	if(_properties.take("nonce").toByteArray() != message.nonce)
		throw ClientException("Invalid nonce in RegisterMessagee");

	_deviceId = message.deviceId;
	_catStr = "client." + _deviceId.toByteArray();
	_logCat.reset(new QLoggingCategory(_catStr.constData()));

	if(name != message.name)
		_database->updateName(_deviceId, message.name);
	qDebug() << "Device successfully logged in";
	sendMessage(serializeMessage(WelcomeMessage()));
	_state = Idle;
}



ClientException::ClientException(const QByteArray &what) :
	_what(what)
{}

const char *ClientException::what() const noexcept
{
	return _what.constData();
}

void ClientException::raise() const
{
	throw *this;
}

QException *ClientException::clone() const
{
	return new ClientException(_what);
}
