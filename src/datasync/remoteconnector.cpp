#include "remoteconnector_p.h"
#include "logger.h"

#include <QtCore/QSysInfo>

#include "registermessage_p.h"
#include "loginmessage_p.h"
#include "syncmessage_p.h"

using namespace QtDataSync;

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

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
	_disconnecting(false),
	_reconnecting(false),
	_state(RemoteDisconnected),
	_retryIndex(0),
	_deviceId()
{}

void RemoteConnector::initialize()
{
	_cryptoController->initialize();

	_pingTimer->setInterval(sValue(keyKeepaliveTimeout).toInt());

	//always "reconnect", because this loads keys etc. and if disabled, also does nothing
	QMetaObject::invokeMethod(this, "reconnect", Qt::QueuedConnection);
}

void RemoteConnector::finalize()
{
	_pingTimer->stop();

	if(_socket && _socket->state() == QAbstractSocket::ConnectedState) {
		_disconnecting = true;
		_socket->close();
		//TODO wait for disconnect?
	}

	_cryptoController->finalize();
}

void RemoteConnector::reconnect()
{
	if(_socket) {
		if(_socket->state() == QAbstractSocket::UnconnectedState) {
			logDebug() << "Removing unconnected but still not deleted socket";
			_socket->deleteLater();
			_socket = nullptr;
			upState(RemoteDisconnected);
			QMetaObject::invokeMethod(this, "reconnect", Qt::QueuedConnection);
		} else if(_socket->state() == QAbstractSocket::ConnectedState) {
			logDebug() << "Closing active connection with server to reconnect";
			_disconnecting = true;
			_reconnecting = true;
			_socket->close();
			upState(RemoteReconnecting);
		} else {
			_reconnecting = true;
			logDebug() << "Delaying reconnect. Socket in changing state:" << _socket->state();
		}
	} else {
		upState(RemoteReconnecting);
		QUrl remoteUrl;
		if(!checkCanSync(remoteUrl))
			return;

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
}

void RemoteConnector::resync()
{
	if(_state != RemoteIdle)
		return;
	upState(RemoteDownloading);
	_socket->sendBinaryMessage(serializeMessage<SyncMessage>(SyncMessage::TriggerAction));
}

void RemoteConnector::connected()
{
	logDebug() << "Successfully connected to remote server";
	upState(RemoteConnected);
}

void RemoteConnector::disconnected()
{
	if(!_disconnecting) {
		if(_state != RemoteReconnecting) {
			logWarning().noquote() << "Unexpected disconnect from server with exit code"
								   << _socket->closeCode()
								   << "and reason:"
								   << _socket->closeReason();
		}
		auto delta = retry();
		logDebug() << "Retrying to connect to server in"
				   << delta.count()
				   << "seconds";
	} else {
		_disconnecting = false;
		logDebug() << "Remote server has been disconnected";
	}

	if(_socket)//better be safe
		_socket->deleteLater();
	_socket = nullptr;
	_cryptoController->clearKeyMaterial();

	//always "disable" remote for the state changer
	upState(RemoteDisconnected);

	if(_reconnecting) {
		_reconnecting = false;
		reconnect();
	}
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
		else if(isType<SyncMessage>(name))
			onSync(deserializeMessage<SyncMessage>(stream));
		else
			logWarning() << "Unknown message received: " << typeName(name);
	} catch(DataStreamException &e) {
		logCritical() << "Remote message error:" << e.what();
	}
}

void RemoteConnector::error(QAbstractSocket::SocketError error)
{
	Q_UNUSED(error)
	if(_retryIndex == 0) {
		logWarning().noquote() << "Server connection socket error:"
							   << _socket->errorString();
	} else {
		logDebug().noquote() << "Repeated server connection socket error:"
							 << _socket->errorString();
	}

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
		if(_retryIndex == 0) {
			logWarning().noquote() << "Server connection SSL error:"
								   << error.errorString();
		} else {
			logDebug().noquote() << "Repeated server connection SSL error:"
								 << error.errorString();
		}
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

bool RemoteConnector::checkCanSync(QUrl &remoteUrl)
{
	//check if sync is enabled
	if(!sValue(keyRemoteEnabled).toBool()) {
		logDebug() << "Remote has been disabled. Not connecting";
		upState(RemoteDisconnected);
		return false;
	}

	//check if remote is defined
	remoteUrl = sValue(keyRemoteUrl).toUrl();
	if(!remoteUrl.isValid()) {
		logDebug() << "Cannot connect to remote - no URL defined";
		upState(RemoteDisconnected);
		return false;
	}

	//load crypto stuff
	if(!loadIdentity()) {
		logCritical() << "Unable to access private keys of user in keystore. Cannot synchronize";
		upState(RemoteDisconnected);
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

void RemoteConnector::upState(RemoteConnector::RemoteState state)
{
	if(state != _state) {
		_state = state;
		emit stateChanged(state);
	}
}

void RemoteConnector::onError(const ErrorMessage &message)
{
	logCritical() << message;
	//TODO emit error to userspace (i.e. sync manager?)
	//TODO special reaction on e.g. Auth Error
	_disconnecting = !message.canRecover;
	tryClose();
}
//TODO reconnect instead of ignore?
void RemoteConnector::onIdentify(const IdentifyMessage &message)
{
	try {
		if(_state != RemoteConnected)
			logWarning() << "Ignoring unexpected IdentifyMessage";
		else {
			if(!_deviceId.isNull()) {
				LoginMessage msg(_deviceId,
								 sValue(keyDeviceName).toString(),
								 message.nonce);
				auto signedMsg = _cryptoController->serializeSignedMessage(msg);
				_socket->sendBinaryMessage(signedMsg);
				logDebug() << "Sent login message for device id" << _deviceId;
				upState(RemoteLoggingIn);
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
				upState(RemoteRegistering);
			}
		}
	} catch(Exception &e) {
		logCritical() << e.what();
	}
}

void RemoteConnector::onAccount(const AccountMessage &message)
{
	try {
		if(_state != RemoteRegistering)
			logWarning() << "Ignoring unexpected AccountMessage";
		else {
			_deviceId = message.deviceId;
			settings()->setValue(keyDeviceId, _deviceId);
			_cryptoController->storePrivateKeys(_deviceId);
			logDebug() << "Saved user data stuff";
			// reset retry index only after successfuly account creation or login
			_retryIndex = 0;
			upState(RemoteIdle);
		}
	} catch(Exception &e) {
		logCritical() << e.what();
	}
}

void RemoteConnector::onWelcome(const WelcomeMessage &message)
{
	Q_UNUSED(message);
	if(_state != RemoteLoggingIn)
		logWarning() << "Ignoring unexpected WelcomeMessage";
	else {
		logDebug() << "Login successful";
		// reset retry index only after successfuly account creation or login
		_retryIndex = 0;
		_state = RemoteIdle;//to allow syncing
		onSync(message);
	}
}

void RemoteConnector::onSync(const SyncMessage &message)
{
	switch (message.action) {
	case SyncMessage::InitAction:
		if(_state != RemoteIdle)
			logWarning() << "Ignoring unexpected SyncMessage::InitAction";
		else {
			logDebug() << "Server has changes. Reloading states";
			upState(RemoteDownloading);
		}
		break;
	case SyncMessage::DoneAction:
		if(_state != RemoteIdle && _state != RemoteDownloading)
			logWarning() << "Ignoring unexpected SyncMessage::DoneAction";
		else {
			//TODO trigger local upload
			logDebug() << "No more changes on server";
			upState(RemoteIdle);
		}
		break;
	default:
		logWarning() << "Ignoring unsupported sync action" << message.action;
		break;
	}
}
