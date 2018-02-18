#include "client.h"
#include "app.h"
#include "identifymessage_p.h"
#include "accountmessage_p.h"
#include "welcomemessage_p.h"
#include "grantmessage_p.h"
#include "devicekeysmessage_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUuid>

#include <QtConcurrent/QtConcurrentRun>

#if QT_HAS_INCLUDE(<chrono>)
#define scdtime(x) x
#else
#define scdtime(x) duration_cast<milliseconds>(x).count()
#endif

using namespace QtDataSync;
using namespace std::chrono;
using std::function;
using std::get;
using std::tie;

#undef qDebug
#define qDebug(...) qCDebug(logFn, __VA_ARGS__)
#undef qInfo
#define qInfo(...) qCInfo(logFn, __VA_ARGS__)
#undef qWarning
#define qWarning(...) qCWarning(logFn, __VA_ARGS__)
#undef qCritical
#define qCritical(...) qCCritical(logFn, __VA_ARGS__)

// ------------- Exceptions Definitions -------------

class MessageException : public QException
{
public:
	explicit MessageException(const QByteArray &message);

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

QThreadStorage<Client::Rng> Client::rngPool;

Client::Client(DatabaseController *database, QWebSocket *websocket, QObject *parent) :
	QObject(parent),
	_catStr(),
	_logCat(new QLoggingCategory("client.unknown")),
	_database(database),
	_socket(websocket),
	_idleTimer(nullptr),
	_uploadLimit(10),
	_downLimit(20),
	_downThreshold(10),
	_queue(new SingleTaskQueue(qApp->threadPool(), this)),
	_state(Authenticating),
	_deviceId(),
	_loginNonce(),
	_cachedChanges(0),
	_activeDownloads()
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

	_uploadLimit = qApp->configuration()->value(QStringLiteral("server/uploads/limit"), _uploadLimit).toUInt();
	_downLimit = qApp->configuration()->value(QStringLiteral("server/downloads/limit"), _downLimit).toUInt();
	_downThreshold = qApp->configuration()->value(QStringLiteral("server/downloads/threshold"), _downThreshold).toUInt();
	auto idleTimeout = qApp->configuration()->value(QStringLiteral("server/idleTimeout"), 5).toInt();
	if(idleTimeout > 0) {
		_idleTimer = new QTimer(this);
		_idleTimer->setInterval(scdtime(minutes(idleTimeout)));
		_idleTimer->setTimerType(Qt::VeryCoarseTimer);
		_idleTimer->setSingleShot(true);
		connect(_idleTimer, &QTimer::timeout,
				this, &Client::timeout);
		_idleTimer->start();
	}

	run([this]() {
		//initialize connection by sending indent message
		auto msg = IdentifyMessage::createRandom(_uploadLimit, rngPool.localData());
		_loginNonce = msg.nonce;
		sendMessage(msg);
	});
}

void Client::dropConnection()
{
	_socket->close();
}

void Client::notifyChanged()
{
	run([this]() {
		if(_state == Idle) //silently ignore other states
			triggerDownload();
	});
}

void Client::proofResult(bool success, const AcceptMessage &message)
{
	run([this, success, message](){
		if(_state != AwatingGrant) {
			qWarning() << "Unexpected proof result. Ignoring";
			return;
		} else {
			qDebug() << "Proof completed with result:" << success;
			if(success) {
				if(message.deviceId != _deviceId) {
					qWarning() << "Message device id does not match. Own id:"
							   << _deviceId << "agains message id:" << message.deviceId;
					return;
				}

				auto pDevId = _cachedAccessRequest.partnerId;
				_database->addNewDeviceToUser(_deviceId,
											  pDevId,
											  _cachedAccessRequest.deviceName,
											  _cachedAccessRequest.signAlgorithm,
											  _cachedAccessRequest.signKey,
											  _cachedAccessRequest.cryptAlgorithm,
											  _cachedAccessRequest.cryptKey,
											  _cachedFingerPrint);
				_cachedAccessRequest = AccessMessage();
				_cachedFingerPrint.clear();

				qDebug() << "Created new device and added to account of device" << pDevId;
				sendMessage(GrantMessage{message});
				_state = Idle;
				emit connected(_deviceId);
			} else
				sendError(ErrorMessage::AccessError);
		}
	});
}

void Client::sendProof(const ProofMessage &message)
{
	run([this, message]() {
		if(_state != Idle) {
			qWarning() << "Cannot send proof when not in idle state";
			emit proofDone(message.deviceId, false);
		} else
			sendMessage(message);
	});
}

void Client::acceptDone(const QUuid &deviceId)
{
	run([this, deviceId]() {
		if(_state != Idle)
			qWarning() << "Cannot send accept ack when not in idle state";
		else
			sendMessage(AcceptAckMessage(deviceId));
	});
}

void Client::binaryMessageReceived(const QByteArray &message)
{
	if(message == Message::PingMessage) {
		if(_idleTimer)
			_idleTimer->start();
		_socket->sendBinaryMessage(Message::PingMessage);
		return;
	}

	run([message, this]() {
		if(_state == Error)
			return;

		try {
			QDataStream stream(message);
			Message::setupStream(stream);
			stream.startTransaction();
			QByteArray name;
			stream >> name;
			if(!stream.commitTransaction())
				throw DataStreamException(stream);

			if(Message::isType<RegisterMessage>(name))
				onRegister(Message::deserializeMessage<RegisterMessage>(stream), stream);
			else if(Message::isType<LoginMessage>(name))
				onLogin(Message::deserializeMessage<LoginMessage>(stream), stream);
			else if(Message::isType<AccessMessage>(name))
				onAccess(Message::deserializeMessage<AccessMessage>(stream), stream);
			else if(Message::isType<SyncMessage>(name))
				onSync(Message::deserializeMessage<SyncMessage>(stream));
			else if(Message::isType<ChangeMessage>(name))
				onChange(Message::deserializeMessage<ChangeMessage>(stream));
			else if(Message::isType<DeviceChangeMessage>(name))
				onDeviceChange(Message::deserializeMessage<DeviceChangeMessage>(stream));
			else if(Message::isType<ChangedAckMessage>(name))
				onChangedAck(Message::deserializeMessage<ChangedAckMessage>(stream));
			else if(Message::isType<ListDevicesMessage>(name))
				onListDevices(Message::deserializeMessage<ListDevicesMessage>(stream));
			else if(Message::isType<RemoveMessage>(name))
				onRemove(Message::deserializeMessage<RemoveMessage>(stream));
			else if(Message::isType<AcceptMessage>(name))
				onAccept(Message::deserializeMessage<AcceptMessage>(stream), stream);
			else if(Message::isType<DenyMessage>(name))
				onDeny(Message::deserializeMessage<DenyMessage>(stream));
			else if(Message::isType<MacUpdateMessage>(name))
				onMacUpdate(Message::deserializeMessage<MacUpdateMessage>(stream));
			else if(Message::isType<KeyChangeMessage>(name))
				onKeyChange(Message::deserializeMessage<KeyChangeMessage>(stream));
			else if(Message::isType<NewKeyMessage>(name))
				onNewKey(Message::deserializeMessage<NewKeyMessage>(stream), stream);
			else {
				qWarning() << "Unknown message received:" << Message::typeName(name);
				sendError({
							  ErrorMessage::IncompatibleVersionError,
							  QStringLiteral("Unknown message type \"%1\"").arg(QString::fromUtf8(Message::typeName(name)))
						  });
			}
		} catch(IncompatibleVersionException &e) {
			qWarning() << "Message error:" << e.what();
			sendError({
						  ErrorMessage::IncompatibleVersionError,
						  QStringLiteral("Version %1 is not compatible to server version %2")
						  .arg(e.invalidVersion().toString())
						  .arg(InitMessage::CurrentVersion.toString())
					  });
		}
	});
}

void Client::error()
{
	qWarning() << "Socket error:"
			   << _socket->errorString();
	_state = Error;
	if(_socket->state() == QAbstractSocket::ConnectedState)
		_socket->close();
}

void Client::sslErrors(const QList<QSslError> &errors)
{
	for(auto error : errors) {
		qWarning() << "SSL error:"
				   << error.errorString();
	}
	_state = Error;
	if(_socket->state() == QAbstractSocket::ConnectedState)
		_socket->close();
}

void Client::closeClient()
{
	if(_queue->clear()) {//save close -> delete only if no parallel stuff anymore (check every 5s)
		qDebug() << "Client disconnected";
		deleteLater();
	} else {
		auto destroyTimer = new QTimer(this);
		destroyTimer->setTimerType(Qt::VeryCoarseTimer);
		connect(destroyTimer, &QTimer::timeout, this, [this](){
			if(_queue->isFinished()) {
				qDebug() << "Client disconnected";
				deleteLater();
			}
		});
		destroyTimer->start(scdtime(seconds(5)));
	}
}

void Client::timeout()
{
	qDebug() << "Disconnecting idle client";
	_state = Error;
	_socket->close();
}

void Client::run(const function<void ()> &fn)
{
	_queue->enqueue([fn, this]() {
		//TODO "log" (hashed) ip on error? to allow blocking???
		try {
			fn();
		} catch (DatabaseException &e) {
			qCritical() << "Internal database error:" << e.what();
			sendError({ErrorMessage::ServerError, QString(), true});
		} catch (ClientErrorException &e) {
			qWarning() << "Message error:" << e.what();
			sendError(e);
		} catch (std::exception &e) {
			qWarning() << "Message error:" << e.what();
			sendError(ErrorMessage::ClientError);
		}
	});
}

const QLoggingCategory &Client::logFn() const
{
	return *_logCat;
}

template<typename TMessage>
void Client::checkIdle(const TMessage &)
{
	if(_state != Idle)
		throw UnexpectedException<TMessage>();
}

void Client::close()
{
	QMetaObject::invokeMethod(_socket, "close", Qt::QueuedConnection);
}

void Client::closeLater()
{
	QTimer::singleShot(scdtime(seconds(10)), _socket, [this](){
		if(_socket && _socket->state() == QAbstractSocket::ConnectedState)
			_socket->close();
	});
}

void Client::sendMessage(const Message &message)
{
	QMetaObject::invokeMethod(this, "doSend", Qt::QueuedConnection,
							  Q_ARG(QByteArray, message.serialize()));
}

void Client::sendError(const ErrorMessage &message)
{
	_state = Error;
	sendMessage(message);
	closeLater();
}

void Client::doSend(const QByteArray &message)
{
	_socket->sendBinaryMessage(message);
}

void Client::onRegister(const RegisterMessage &message, QDataStream &stream)
{
	if(_state != Authenticating)
		throw UnexpectedException<RegisterMessage>();
	if(_loginNonce != message.nonce)
		throw MessageException("Invalid nonce in RegisterMessagee");
	_loginNonce.clear();

	try {
		QScopedPointer<AsymmetricCryptoInfo> crypto(message.createCryptoInfo(rngPool.localData()));
		Message::verifySignature(stream, crypto->signatureKey(), crypto.data());

		_deviceId = _database->addNewDevice(message.deviceName,
											message.signAlgorithm,
											message.signKey,
											message.cryptAlgorithm,
											message.cryptKey,
											crypto->ownFingerprint(),
											message.cmac);
	} catch(CryptoPP::SignatureVerificationFilter::SignatureVerificationFailed &e) {
		qWarning() << "Authentication error:" << e.what();
		throw ClientErrorException(ErrorMessage::AuthenticationError);
	}

	_catStr = "client." + _deviceId.toByteArray();
	_logCat.reset(new QLoggingCategory(_catStr.constData()));

	qDebug() << "Created new device and user accounts";
	sendMessage(AccountMessage{_deviceId});
	_state = Idle;
	emit connected(_deviceId);
}

void Client::onLogin(const LoginMessage &message, QDataStream &stream)
{
	if(_state != Authenticating)
		throw UnexpectedException<LoginMessage>();
	if(_loginNonce != message.nonce)
		throw MessageException("Invalid nonce in LoginMessage");
	_loginNonce.clear();

	//load public key to verify signature
	try {
		QScopedPointer<AsymmetricCryptoInfo> crypto(_database->loadCrypto(message.deviceId, rngPool.localData()));
		if(!crypto)
			throw ClientErrorException(ErrorMessage::AuthenticationError);
		Message::verifySignature(stream, crypto->signatureKey(), crypto.data());
	} catch(CryptoPP::SignatureVerificationFilter::SignatureVerificationFailed &e) {
		qWarning() << "Authentication error:" << e.what();
		throw ClientErrorException(ErrorMessage::AuthenticationError);
	}

	_deviceId = message.deviceId;
	_catStr = "client." + _deviceId.toByteArray();
	_logCat.reset(new QLoggingCategory(_catStr.constData()));

	_database->updateLogin(_deviceId, message.deviceName);
	qDebug() << "Device successfully logged in";

	//load changecount early to find out if data changed
	_cachedChanges = _database->changeCount(_deviceId);
	WelcomeMessage reply(_cachedChanges > 0);
	tie(reply.keyIndex, reply.scheme, reply.key, reply.cmac) = _database->loadKeyChanges(_deviceId);
	sendMessage(reply);
	_state = Idle;
	emit connected(_deviceId);

	// send changed, always send info msg first, because count was preloaded (no force)
	// in case of no changes, send nothing if no changes
	triggerDownload(true, _cachedChanges == 0);
}

void Client::onAccess(const AccessMessage &message, QDataStream &stream)
{
	if(_state != Authenticating)
		throw UnexpectedException<AccessMessage>();
	if(_loginNonce != message.nonce)
		throw MessageException("Invalid nonce in AccessMessage");
	_loginNonce.clear();

	try {
		QScopedPointer<AsymmetricCryptoInfo> crypto(message.createCryptoInfo(rngPool.localData()));
		Message::verifySignature(stream, crypto->signatureKey(), crypto.data());
		_cachedFingerPrint = crypto->ownFingerprint();
	} catch(CryptoPP::SignatureVerificationFilter::SignatureVerificationFailed &e) {
		qWarning() << "Authentication error:" << e.what();
		throw ClientErrorException(ErrorMessage::AuthenticationError);
	}

	_deviceId = QUuid::createUuid(); //not stored yet!!!
	_cachedAccessRequest = message;
	//_cachedFingerPrint done inside of try/catch block
	_catStr = "client." + _deviceId.toByteArray();
	_logCat.reset(new QLoggingCategory(_catStr.constData()));

	qDebug() << "New Devices requested account access from" << message.partnerId;
	_state = AwatingGrant;
	emit proofRequested(message.partnerId, ProofMessage{message, _deviceId});
}

void Client::onSync(const SyncMessage &message)
{
	checkIdle(message);
	triggerDownload();
}

void Client::onChange(const ChangeMessage &message)
{
	checkIdle(message);

	if(_database->addChange(_deviceId,
							message.dataId,
							message.keyIndex,
							message.salt,
							message.data))
		sendMessage(ChangeAckMessage{message});
	else
		sendError(ErrorMessage::QuotaHitError);
}

void Client::onDeviceChange(const DeviceChangeMessage &message)
{
	checkIdle(message);

	if(_database->addDeviceChange(_deviceId,
								  message.deviceId,
								  message.dataId,
								  message.keyIndex,
								  message.salt,
								  message.data))
		sendMessage(DeviceChangeAckMessage{message});
	else
		sendError(ErrorMessage::QuotaHitError);

}

void Client::onChangedAck(const ChangedAckMessage &message)
{
	checkIdle(message);

	_database->completeChange(_deviceId, message.dataIndex);
	_activeDownloads.removeOne(message.dataIndex);
	//trigger next download. method itself decides when and how etc.
	triggerDownload();
}

void Client::onListDevices(const ListDevicesMessage &message)
{
	Q_UNUSED(message);
	checkIdle(message);

	DevicesMessage devMessage;
	for(auto device : _database->listDevices(_deviceId))
		devMessage.devices.append(device);

	sendMessage(devMessage);
}

void Client::onRemove(const RemoveMessage &message)
{
	checkIdle(message);

	_database->removeDevice(_deviceId, message.deviceId);
	sendMessage(RemoveAckMessage{message.deviceId});
	if(_deviceId == message.deviceId) {
		qDebug() << "Removed self from account";
		_state = Error;
		closeLater(); //in case the client does not get the message...
	} else {
		qDebug() << "Removed device from account:" << message.deviceId;
		emit forceDisconnect(message.deviceId);
	}
}

void Client::onAccept(const AcceptMessage &message, QDataStream &stream)
{
	checkIdle(message);

	//verify the signature (in case of an unsecure channel)
	try {
		QScopedPointer<AsymmetricCryptoInfo> crypto(_database->loadCrypto(_deviceId, rngPool.localData()));
		if(!crypto)
			throw ClientErrorException(ErrorMessage::AuthenticationError);
		Message::verifySignature(stream, crypto->signatureKey(), crypto.data());
	} catch(CryptoPP::SignatureVerificationFilter::SignatureVerificationFailed &e) {
		qWarning() << "Authentication error:" << e.what();
		throw ClientErrorException(ErrorMessage::AuthenticationError);
	}

	emit proofDone(message.deviceId, true, message);
}

void Client::onDeny(const DenyMessage &message)
{
	checkIdle(message);
	emit proofDone(message.deviceId, false);
}

void Client::onMacUpdate(const MacUpdateMessage &message)
{
	checkIdle(message);

	if(_database->updateCmac(_deviceId, message.keyIndex, message.cmac))
		sendMessage(MacUpdateAckMessage());
	else
		sendError(ErrorMessage::KeyIndexError);
}

void Client::onKeyChange(const KeyChangeMessage &message)
{
	checkIdle(message);

	auto offset = 0;
	auto deviceInfos = _database->tryKeyChange(_deviceId, message.nextIndex, offset);
	if(offset == 1) //accepted
		sendMessage(DeviceKeysMessage{message.nextIndex, deviceInfos});
	else if(offset == 0) //proposed is the same as current (accept as duplicate, but don't send any devices)
		sendMessage(DeviceKeysMessage{message.nextIndex});
	else if(offset == -1) //most likely because of client key conflict
		sendError(ErrorMessage::KeyPendingError);
	else
		sendError(ErrorMessage::KeyIndexError);
}

void Client::onNewKey(const NewKeyMessage &message, QDataStream &stream)
{
	checkIdle(message);

	//verify the signature (in case of an unsecure channel)
	try {
		QScopedPointer<AsymmetricCryptoInfo> crypto(_database->loadCrypto(_deviceId, rngPool.localData()));
		if(!crypto)
			throw ClientErrorException(ErrorMessage::AuthenticationError);
		Message::verifySignature(stream, crypto->signatureKey(), crypto.data());
	} catch(CryptoPP::SignatureVerificationFilter::SignatureVerificationFailed &e) {
		qWarning() << "Authentication error:" << e.what();
		throw ClientErrorException(ErrorMessage::AuthenticationError);
	}

	auto ok = _database->updateExchangeKey(_deviceId,
										  message.keyIndex,
										  message.scheme,
										  message.cmac,
										  message.deviceKeys);
	if(ok) {
		for(auto info : message.deviceKeys)
			emit forceDisconnect(get<0>(info));
		sendMessage(NewKeyAckMessage{message});
	} else
		sendError(ErrorMessage::KeyIndexError);
}

void Client::triggerDownload(bool forceUpdate, bool skipNoChanges)
{
	auto updateChange = forceUpdate;

	auto cnt = _downLimit - _activeDownloads.size();
	if(cnt >= _downThreshold) {
		auto changes = _database->loadNextChanges(_deviceId, cnt, _activeDownloads.size());
		for(auto change : changes) {
			if(_cachedChanges == 0) {
				updateChange = true;
				_cachedChanges = _database->changeCount(_deviceId) - _activeDownloads.size();
			}

			if(updateChange) {
				ChangedInfoMessage message(_cachedChanges);
				tie(message.dataIndex, message.keyIndex, message.salt, message.data) = change;
				sendMessage(ChangedInfoMessage{message});
				updateChange = false; //only the first message has that info
			} else {
				ChangedMessage message;
				tie(message.dataIndex, message.keyIndex, message.salt, message.data) = change;
				sendMessage(ChangedMessage{message});
			}
			_activeDownloads.append(get<0>(change));
			_cachedChanges--;
		}
	}

	if(_activeDownloads.isEmpty() && !skipNoChanges) {
		_cachedChanges = 0; //to make shure the next message is a ChangedInfoMessage
		sendMessage(LastChangedMessage());
	}
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
