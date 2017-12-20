#include "client.h"
#include "app.h"
#include "identifymessage_p.h"
#include "accountmessage_p.h"
#include "welcomemessage_p.h"
#include "errormessage_p.h"

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

// ------------- Exceptions Definitions -------------

class MessageException : public QException
{
public:
	MessageException(const QByteArray &message);

	const char *what() const noexcept override;
	void raise() const override;
	QException *clone() const override;

private:
	const QByteArray _msg;
};

class ClientErrorException : public QException, public ErrorMessage
{
public:
	ClientErrorException(ErrorType type = UnknownError,
						 const QString &message = {},
						 bool canRecover = false);

	const char *what() const noexcept override;
	void raise() const override;
	QException *clone() const override;

private:
	mutable QByteArray _msg;
};

template <typename TMessage>
class UnexpectedException : public ClientErrorException
{
	static_assert(std::is_void<typename TMessage::QtGadgetHelper>::value, "Only Q_GADGETS can be serialized");
public:
	UnexpectedException();
};

// ------------- Client Implementation -------------

QThreadStorage<CryptoPP::AutoSeededRandomPool> Client::rngPool;

Client::Client(DatabaseController *database, QWebSocket *websocket, QObject *parent) :
	QObject(parent),
	_catStr(),
	_logCat(new QLoggingCategory("client.unknown")),
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
		try {
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
			} else if(isType<SyncMessage>(name)) {
				auto msg = deserializeMessage<SyncMessage>(stream);
				onSync(msg);
			} else {
				qWarning() << "Unknown message received:" << typeName(name);
				sendMessage(serializeMessage<ErrorMessage>({
															   ErrorMessage::IncompatibleVersionError,
															   QStringLiteral("Unknown message type \"%1\"").arg(QString::fromUtf8(typeName(name)))
														   }));
				closeLater();
			}
		} catch(IncompatibleVersionException &e) {
			qWarning() << "Message error:" << e.what();
			sendMessage(serializeMessage<ErrorMessage>({
														   ErrorMessage::IncompatibleVersionError,
														   QStringLiteral("Version %1 is not compatible to server version %2")
														   .arg(e.invalidVersion().toString())
														   .arg(IdentifyMessage::CurrentVersion.toString())
													   }));
			closeLater();
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
		destroyTimer->start(std::chrono::seconds(5));
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
		//TODO "log" (hashed) ip on error? to allow blocking???
		try {
			fn();
			_runCount--;
		} catch (DatabaseException &e) {
			qCritical() << "Internal database error:" << e.what();
			sendMessage(serializeMessage<ErrorMessage>({ErrorMessage::ServerError, QString(), true}));
			closeLater();
			_runCount--;
		} catch (ClientErrorException &e) {
			qWarning() << "Message error:" << e.what();
			sendMessage(serializeMessage<ErrorMessage>(e));
			closeLater();
			_runCount--;
		} catch (std::exception &e) {
			qWarning() << "Message error:" << e.what();
			sendMessage(serializeMessage<ErrorMessage>({ErrorMessage::ClientError}));
			closeLater();
			_runCount--;
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

void Client::closeLater()
{
	QTimer::singleShot(std::chrono::seconds(10), _socket, [this](){
		if(_socket && _socket->state() == QAbstractSocket::ConnectedState)
			_socket->close();
	});
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
		throw UnexpectedException<RegisterMessage>();
	if(_properties.take("nonce").toByteArray() != message.nonce)
		throw MessageException("Invalid nonce in RegisterMessagee");

	try {
		QScopedPointer<AsymmetricCryptoInfo> crypto(message.createCryptoInfo(rngPool.localData()));
		verifySignature(stream, crypto->signatureKey(), crypto.data());
	} catch(CryptoPP::HashVerificationFilter::HashVerificationFailed &e) {
		qWarning() << "Authentication error:" << e.what();
		throw ClientErrorException(ErrorMessage::AuthenticationError);
	}

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
		throw UnexpectedException<LoginMessage>();
	if(_properties.take("nonce").toByteArray() != message.nonce)
		throw MessageException("Invalid nonce in LoginMessage");

	//load public key to verify signature
	QString name;
	try {
		QScopedPointer<AsymmetricCryptoInfo> crypto(_database->loadCrypto(message.deviceId, rngPool.localData(), name));
		if(!crypto)
			throw ClientErrorException(ErrorMessage::AuthenticationError);
		verifySignature(stream, crypto->signatureKey(), crypto.data());
	} catch(CryptoPP::HashVerificationFilter::HashVerificationFailed &e) {
		qWarning() << "Authentication error:" << e.what();
		throw ClientErrorException(ErrorMessage::AuthenticationError);
	}

	_deviceId = message.deviceId;
	_catStr = "client." + _deviceId.toByteArray();
	_logCat.reset(new QLoggingCategory(_catStr.constData()));

	if(name != message.name)
		_database->updateName(_deviceId, message.name);
	qDebug() << "Device successfully logged in";

	auto change = _database->loadNextChange(_deviceId);
	sendMessage(serializeMessage(WelcomeMessage(change)));
	_state = Idle;

	//TODO implement changes
}

void Client::onSync(const SyncMessage &message)
{
	Q_UNIMPLEMENTED();
}

// ------------- Exceptions Implementation -------------

MessageException::MessageException(const QByteArray &message) :
	_msg(message)
{}

const char *MessageException::what() const noexcept
{
	return _msg.constData();
}

void MessageException::raise() const
{
	throw (*this);
}

QException *MessageException::clone() const
{
	return new MessageException(_msg);
}



ClientErrorException::ClientErrorException(ErrorMessage::ErrorType type, const QString &message, bool canRecover) :
	QException(),
	ErrorMessage(type, message, canRecover)
{}

const char *ClientErrorException::what() const noexcept
{
	QString s;
	QDebug(&s) << ErrorMessage(*this);
	_msg = s.toUtf8();
	return _msg.constData();
}

void ClientErrorException::raise() const
{
	throw *this;
}

QException *ClientErrorException::clone() const
{
	return new ClientErrorException(type, message, canRecover);
}



template<typename TMessage>
UnexpectedException<TMessage>::UnexpectedException() :
	ClientErrorException(UnexpectedMessageError,
						 QStringLiteral("Received unexpected %1").arg(QString::fromUtf8(TMessage::staticMetaObject.className())),
						 true)
{}
