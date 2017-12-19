#include "remoteconnector_p.h"
#include "logger.h"

#include <QtCore/QSysInfo>

#include "registermessage_p.h"
#include "loginmessage_p.h"
#include "syncmessage_p.h"

using namespace QtDataSync;

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

#define logRetry(...) (_retryIndex == 0 ? logWarning(__VA_ARGS__) : (logDebug(__VA_ARGS__) << "Repeated"))

const QString RemoteConnector::keyRemoteEnabled(QStringLiteral("enabled"));
const QString RemoteConnector::keyRemoteUrl(QStringLiteral("remote/url"));
const QString RemoteConnector::keyAccessKey(QStringLiteral("remote/accessKey"));
const QString RemoteConnector::keyHeaders(QStringLiteral("remote/headers"));
const QString RemoteConnector::keyKeepaliveTimeout(QStringLiteral("remote/keepaliveTimeout"));
const QString RemoteConnector::keyDeviceId(QStringLiteral("deviceId"));
const QString RemoteConnector::keyDeviceName(QStringLiteral("deviceName"));

const QVector<std::chrono::seconds> RemoteConnector::Timeouts = {
	std::chrono::seconds(5),
	std::chrono::seconds(10),
	std::chrono::seconds(30),
	std::chrono::minutes(1),
	std::chrono::minutes(5)
};

RemoteConnector::RemoteConnector(const Defaults &defaults, QObject *parent) :
	Controller("connector", defaults, parent),
	_cryptoController(new CryptoController(defaults, this)),
	_socket(nullptr),
	_pingTimer(new QTimer(this)),
	_awaitingPing(false),
	_stateMachine(new ConnectorStateMachine(this)),
	_retryIndex(0),
	_deviceId()
{}

void RemoteConnector::initialize()
{
	_cryptoController->initialize();

	_pingTimer->setInterval(sValue(keyKeepaliveTimeout).toInt());

	//setup SM
	_stateMachine->connectToState(QStringLiteral("Connecting"),
								  ConnectorStateMachine::onEntry(this, &RemoteConnector::doConnect));
	_stateMachine->connectToState(QStringLiteral("Retry"),
								  ConnectorStateMachine::onEntry(this, &RemoteConnector::scheduleRetry));
	_stateMachine->connectToEvent(QStringLiteral("doDisconnect"),
								  this, &RemoteConnector::doDisconnect);
#ifndef QT_NO_DEBUG
	connect(_stateMachine, &ConnectorStateMachine::reachedStableState, this, [this](){
		logDebug() << "Reached stable states:" << _stateMachine->activeStateNames(false);
	});
#endif
	if(!_stateMachine->init())
		throw Exception(defaults(), QStringLiteral("Failed to initialize RemoteConnector statemachine"));

	//always "reconnect", because this loads keys etc. and if disabled, also does nothing
	QMetaObject::invokeMethod(_stateMachine, "start", Qt::QueuedConnection);
}

void RemoteConnector::finalize()
{
	_pingTimer->stop();
	_cryptoController->finalize();

	if(_stateMachine->isRunning()) {
		connect(_stateMachine, &ConnectorStateMachine::finished,
				this, [this](){
			emit finalized();
		});
		_stateMachine->dataModel()->setScxmlProperty(QStringLiteral("isClosing"),
													 true,
													 QStringLiteral("close"));
		//send "dummy" event to revalute the changed properties and trigger the changes
		_stateMachine->submitEvent(QStringLiteral("close"));

		//TODO proper timeout?
		QTimer::singleShot(std::chrono::seconds(2), this, [this](){
			if(_stateMachine->isRunning())
				_stateMachine->stop();
			if(_socket)
				_socket->close();
			emit finalized();
		});
	}
}

void RemoteConnector::reconnect()
{
	_stateMachine->submitEvent(QStringLiteral("reconnect"));
}

void RemoteConnector::resync()
{
	if(!isIdle())
		return;
	//TODO enter "downloading" state
	_socket->sendBinaryMessage(serializeMessage(SyncMessage()));
}

void RemoteConnector::connected()
{
	logDebug() << "Successfully connected to remote server";
	_stateMachine->submitEvent(QStringLiteral("connected"));
}

void RemoteConnector::disconnected()
{
	if(_stateMachine->isActive(QStringLiteral("Active"))) {
		if(_stateMachine->isActive(QStringLiteral("Connecting")))
			logRetry() << "Failed to connect to server";
		else {
			logRetry().noquote() << "Unexpected disconnect from server with exit code"
								   << _socket->closeCode()
								   << "and reason:"
								   << _socket->closeReason();
		}
	} else
		logDebug() << "Remote server has been disconnected";
	if(_socket) { //better be safe
		_socket->disconnect(this);
		_socket->deleteLater();
	}
	_socket = nullptr;
	_cryptoController->clearKeyMaterial();
	_stateMachine->submitEvent(QStringLiteral("disconnected"));
}

void RemoteConnector::binaryMessageReceived(const QByteArray &message)
{
	if(message == PingMessage) {
		_awaitingPing = false;
		_pingTimer->start();
		return;
	}

	try {
		QDataStream stream(message);
		setupStream(stream);
		stream.startTransaction();
		QByteArray name;
		stream >> name;
		if(!stream.commitTransaction())
			throw DataStreamException(stream);

		if(isType<ErrorMessage>(name))
			onError(deserializeMessage<ErrorMessage>(stream));
		else if(isType<IdentifyMessage>(name))
			onIdentify(deserializeMessage<IdentifyMessage>(stream));
		else if(isType<AccountMessage>(name))
			onAccount(deserializeMessage<AccountMessage>(stream));
		else if(isType<WelcomeMessage>(name))
			onWelcome(deserializeMessage<WelcomeMessage>(stream));
		else
			logWarning() << "Unknown message received: " << typeName(name);
	} catch(DataStreamException &e) {
		logCritical() << "Remote message error:" << e.what();
	}
}

void RemoteConnector::error(QAbstractSocket::SocketError error)
{
	Q_UNUSED(error)
	logRetry().noquote() << "Server connection socket error:"
						 << _socket->errorString();

	tryClose();
}

void RemoteConnector::sslErrors(const QList<QSslError> &errors)
{
	auto shouldClose = true;
	foreach(auto error, errors) {
		if(error.error() == QSslError::SelfSignedCertificate ||
		   error.error() == QSslError::SelfSignedCertificateInChain)
			shouldClose = shouldClose &&
						  (defaults().property(Defaults::SslConfiguration)
						   .value<QSslConfiguration>()
						   .peerVerifyMode() >= QSslSocket::VerifyPeer);
		logRetry().noquote() << "Server connection SSL error:"
							   << error.errorString();
	}

	if(shouldClose)
		tryClose();
}

void RemoteConnector::ping()
{
	if(_awaitingPing) {
		_awaitingPing = false;
		logDebug() << "Server connection idle. Reconnecting to server";
		reconnect();
	} else {
		_awaitingPing = true;
		_socket->sendBinaryMessage(PingMessage);
	}
}

void RemoteConnector::doConnect()
{
	QUrl remoteUrl;
	if(!checkCanSync(remoteUrl)) {
		_stateMachine->submitEvent(QStringLiteral("noConnect"));
		return;
	}

	if(_socket && _socket->state() != QAbstractSocket::UnconnectedState) {
		logWarning() << "Deleting already open socket connection";
		_socket->disconnect(this);
		_socket->deleteLater();
	}
	_socket = new QWebSocket(sValue(keyAccessKey).toString(),
							 QWebSocketProtocol::VersionLatest,
							 this);

	auto conf = defaults().property(Defaults::SslConfiguration).value<QSslConfiguration>();
	if(!conf.isNull())
		_socket->setSslConfiguration(conf);

	connect(_socket, &QWebSocket::connected,
			this, &RemoteConnector::connected);
	connect(_socket, &QWebSocket::binaryMessageReceived,
			this, &RemoteConnector::binaryMessageReceived);
	connect(_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
			this, &RemoteConnector::error);
	connect(_socket, &QWebSocket::sslErrors,
			this, &RemoteConnector::sslErrors);
	connect(_socket, &QWebSocket::disconnected,
			this, &RemoteConnector::disconnected,
			Qt::QueuedConnection);

	//initialize keep alive timeout
	auto tOut = sValue(keyKeepaliveTimeout).toInt();
	if(tOut > 0) {
		_pingTimer->setInterval(std::chrono::minutes(tOut));
		connect(_socket, &QWebSocket::connected,
				_pingTimer, QOverload<>::of(&QTimer::start));
		connect(_socket, &QWebSocket::disconnected,
				_pingTimer, &QTimer::stop);
		connect(_pingTimer, &QTimer::timeout,
				this, &RemoteConnector::ping);
	}

	QNetworkRequest request(remoteUrl);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
	request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
	request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
	request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);

	auto keys = sValue(keyHeaders).value<QHash<QByteArray, QByteArray>>();
	for(auto it = keys.begin(); it != keys.end(); it++)
		request.setRawHeader(it.key(), it.value());

	_socket->open(request);
	logDebug() << "Connecting to remote server...";
}

void RemoteConnector::doDisconnect()
{
	if(_socket) {
		switch (_socket->state()) {
		case QAbstractSocket::HostLookupState:
		case QAbstractSocket::ConnectingState:
			logWarning() << "Trying to disconnect while connecting. Connection will be discarded without proper disconnecting";
			Q_FALLTHROUGH();
		case QAbstractSocket::UnconnectedState:
			logDebug() << "Removing unconnected but still not deleted socket";
			_socket->disconnect(this);
			_socket->deleteLater();
			_socket = nullptr;
			_stateMachine->submitEvent(QStringLiteral("disconnected"));
			break;
		case QAbstractSocket::ClosingState:
			logDebug() << "Already disconnecting. Doing nothing";
			break;
		case QAbstractSocket::ConnectedState:
			logDebug() << "Closing active connection with server";
			_socket->close();
			break;
		case QAbstractSocket::BoundState:
		case QAbstractSocket::ListeningState:
			logFatal("Reached impossible client socket state - how?!?");
			break;
		default:
			Q_UNREACHABLE();
			break;
		}
	} else
		_stateMachine->submitEvent(QStringLiteral("disconnected"));
}

void RemoteConnector::scheduleRetry()
{
	auto delta = retry();
	logDebug() << "Retrying to connect to server in"
			   << std::chrono::duration_cast<std::chrono::seconds>(delta).count()
			   << "seconds";
}

bool RemoteConnector::isIdle() const
{
	return _stateMachine->isActive(QStringLiteral("Idle"));
}

void RemoteConnector::triggerError(bool canRecover)
{
	if(canRecover)
		_stateMachine->submitEvent(QStringLiteral("basicError"));
	else
		_stateMachine->submitEvent(QStringLiteral("fatalError"));
}

bool RemoteConnector::checkCanSync(QUrl &remoteUrl)
{
	//test not closing
	if(_stateMachine->dataModel()->scxmlProperty(QStringLiteral("isClosing")).toBool())
		return false;

	//check if sync is enabled
	if(!sValue(keyRemoteEnabled).toBool()) {
		logDebug() << "Remote has been disabled. Not connecting";
		return false;
	}

	//check if remote is defined
	remoteUrl = sValue(keyRemoteUrl).toUrl();
	if(!remoteUrl.isValid()) {
		logDebug() << "Cannot connect to remote - no URL defined";
		return false;
	}

	//load crypto stuff
	if(!loadIdentity()) {
		logCritical() << "Unable to access private keys of user in keystore. Cannot synchronize";
		return false;
	}

	return true;
}

bool RemoteConnector::loadIdentity()
{
	try {
		if(!_cryptoController->canAccessStore()) //no keystore -> can neither save nor load...
			return false;

		_deviceId = sValue(keyDeviceId).toUuid();
		if(_deviceId.isNull()) //no user -> nothing to be loaded
			return true;

		_cryptoController->loadKeyMaterial(_deviceId);
		return true;
	} catch(Exception &e) {
		logCritical() << e.what();
		return false;
	}
}

void RemoteConnector::tryClose()
{
	// do not set _disconnecting, because this is unexpected
	if(_socket && _socket->state() == QAbstractSocket::ConnectedState)
		_socket->close();
}

std::chrono::seconds RemoteConnector::retry()
{
	std::chrono::seconds retryTimeout;
	if(_retryIndex >= Timeouts.size())
		retryTimeout = Timeouts.last();
	else
		retryTimeout = Timeouts[_retryIndex++];

	QTimer::singleShot(retryTimeout, this, &RemoteConnector::reconnect);

	return retryTimeout;
}

QVariant RemoteConnector::sValue(const QString &key) const
{
	if(key == keyHeaders) {
		if(settings()->childGroups().contains(keyHeaders)) {
			settings()->beginGroup(keyHeaders);
			auto keys = settings()->childKeys();
			QHash<QByteArray, QByteArray> headers;
			foreach(auto key, keys)
				headers.insert(key.toUtf8(), settings()->value(key).toByteArray());
			settings()->endGroup();
			return QVariant::fromValue(headers);
		}
	} else {
		auto res = settings()->value(key);
		if(res.isValid())
			return res;
	}

	auto config = defaults().property(Defaults::RemoteConfiguration).value<RemoteConfig>();
	if(key == keyRemoteUrl)
		return config.url;
	else if(key == keyAccessKey)
		return config.accessKey;
	else if(key == keyHeaders)
		return QVariant::fromValue(config.headers);
	else if(key == keyKeepaliveTimeout)
		return QVariant::fromValue(config.keepaliveTimeout);
	else if(key == keyRemoteEnabled)
		return true;
	else if(key == keyDeviceName)
		return QSysInfo::machineHostName();
	else
		return {};
}

void RemoteConnector::onError(const ErrorMessage &message)
{
	logCritical() << message;
	//TODO emit error to userspace (i.e. sync manager?)
	//TODO special reaction on e.g. Auth Error
	triggerError(message.canRecover);
}

void RemoteConnector::onIdentify(const IdentifyMessage &message)
{
	try {
		if(!_stateMachine->isActive(QStringLiteral("Connected"))) {
			logWarning() << "Unexpected IdentifyMessage";
			triggerError(true);
		} else {
			if(!_deviceId.isNull()) {
				LoginMessage msg(_deviceId,
								 sValue(keyDeviceName).toString(),
								 message.nonce);
				auto signedMsg = _cryptoController->serializeSignedMessage(msg);
				_socket->sendBinaryMessage(signedMsg);
				logDebug() << "Sent login message for device id" << _deviceId;
				_stateMachine->submitEvent(QStringLiteral("awaitLogin"));
			} else {
				_cryptoController->createPrivateKeys(message.nonce);
				RegisterMessage msg(sValue(keyDeviceName).toString(),
									message.nonce,
									_cryptoController->crypto()->signKey(),
									_cryptoController->crypto()->cryptKey(),
									_cryptoController->crypto());
				auto signedMsg = _cryptoController->serializeSignedMessage(msg);
				_socket->sendBinaryMessage(signedMsg);
				logDebug() << "Sent registration message for new id";
				_stateMachine->submitEvent(QStringLiteral("awaitRegister"));
			}
		}
	} catch(Exception &e) {
		logCritical() << e.what();
	}
}

void RemoteConnector::onAccount(const AccountMessage &message)
{
	try {
		if(!_stateMachine->isActive(QStringLiteral("Registering"))) {
			logWarning() << "Unexpected AccountMessage";
			triggerError(true);
		} else {
			_deviceId = message.deviceId;
			settings()->setValue(keyDeviceId, _deviceId);
			_cryptoController->storePrivateKeys(_deviceId);
			logDebug() << "Registration successful";
			// reset retry index only after successfuly account creation or login
			_retryIndex = 0;
			_stateMachine->submitEvent(QStringLiteral("account"));
		}
	} catch(Exception &e) {
		logCritical() << e.what();
	}
}

void RemoteConnector::onWelcome(const WelcomeMessage &message)
{
	if(!_stateMachine->isActive(QStringLiteral("LoggingIn"))) {
		logWarning() << "Unexpected WelcomeMessage";
		triggerError(true);
	} else {
		logDebug() << "Login successful";
		// reset retry index only after successfuly account creation or login
		_retryIndex = 0;
		_stateMachine->submitEvent(QStringLiteral("account"));
		if(message.hasChanges) {
			logDebug() << "Server has changes. Reloading states";
			//TODO enter "downloading" state
		} else {

		}
	}
}
