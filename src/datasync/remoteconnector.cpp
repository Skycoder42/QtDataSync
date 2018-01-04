#include "remoteconnector_p.h"
#include "logger.h"

#include <QtCore/QSysInfo>

#include "registermessage_p.h"
#include "loginmessage_p.h"
#include "syncmessage_p.h"

#include "connectorstatemachine.h"

#if QT_HAS_INCLUDE(<chrono>)
#define scdtime(x) x
#else
#define scdtime(x) std::chrono::duration_cast<std::chrono::milliseconds>(x).count()
#endif

using namespace QtDataSync;

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

#define logRetry(...) (_retryIndex == 0 ? logWarning(__VA_ARGS__) : (logDebug(__VA_ARGS__) << "Repeated"))

const QString RemoteConnector::keyRemoteEnabled(QStringLiteral("enabled"));
const QString RemoteConnector::keyRemoteConfig(QStringLiteral("remote"));
const QString RemoteConnector::keyRemoteUrl(QStringLiteral("remote/url"));
const QString RemoteConnector::keyAccessKey(QStringLiteral("remote/accessKey"));
const QString RemoteConnector::keyHeaders(QStringLiteral("remote/headers"));
const QString RemoteConnector::keyKeepaliveTimeout(QStringLiteral("remote/keepaliveTimeout"));
const QString RemoteConnector::keyDeviceId(QStringLiteral("deviceId"));
const QString RemoteConnector::keyDeviceName(QStringLiteral("deviceName"));
const QString RemoteConnector::keyImport(QStringLiteral("import"));
const QString RemoteConnector::keyImportTrusted(QStringLiteral("import/trusted"));
const QString RemoteConnector::keyImportNonce(QStringLiteral("import/nonce"));
const QString RemoteConnector::keyImportPartner(QStringLiteral("import/partner"));
const QString RemoteConnector::keyImportScheme(QStringLiteral("import/scheme"));
const QString RemoteConnector::keyImportCmac(QStringLiteral("import/cmac"));

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
	_pingTimer(nullptr),
	_awaitingPing(false),
	_stateMachine(nullptr),
	_retryIndex(0),
	_expectChanges(false),
	_deviceId(),
	_deviceCache()
{}

CryptoController *RemoteConnector::cryptoController() const
{
	return _cryptoController;
}

void RemoteConnector::initialize(const QVariantHash &params)
{
	_cryptoController->initialize(params);

	//setup keepalive timer
	_pingTimer = new QTimer(this);
	_pingTimer->setInterval(sValue(keyKeepaliveTimeout).toInt());
	_pingTimer->setTimerType(Qt::VeryCoarseTimer);

	//setup SM
	_stateMachine = new ConnectorStateMachine(this);
	_stateMachine->connectToState(QStringLiteral("Connecting"),
								  this, ConnectorStateMachine::onEntry(this, &RemoteConnector::doConnect));
	_stateMachine->connectToState(QStringLiteral("Retry"),
								  this, ConnectorStateMachine::onEntry(this, &RemoteConnector::scheduleRetry));
	_stateMachine->connectToState(QStringLiteral("Idle"),
								  this, ConnectorStateMachine::onEntry(this, &RemoteConnector::onEntryIdleState));
	_stateMachine->connectToState(QStringLiteral("Active"),
								  this, ConnectorStateMachine::onExit(this, &RemoteConnector::onExitActiveState));
	_stateMachine->connectToEvent(QStringLiteral("doDisconnect"),
								  this, &RemoteConnector::doDisconnect);
#ifndef QT_NO_DEBUG
	connect(_stateMachine, &ConnectorStateMachine::reachedStableState, this, [this](){
		logDebug() << "Reached stable states:" << _stateMachine->activeStateNames(false);
	});
#endif
	if(!_stateMachine->init())
		throw Exception(defaults(), QStringLiteral("Failed to initialize RemoteConnector statemachine"));

	_stateMachine->start();
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
		QTimer::singleShot(scdtime(std::chrono::seconds(2)), this, [this](){
			if(_stateMachine->isRunning())
				_stateMachine->stop();
			if(_socket)
				_socket->close();
			emit finalized();
		});
	} else
		emit finalized();
}

std::tuple<ExportData, QByteArray, CryptoPP::SecByteBlock> RemoteConnector::exportAccount(bool includeServer, const QString &password)
{
	if(_deviceId.isNull())
		throw Exception(defaults(), QStringLiteral("Cannot export data without beeing registered on a server."));

	ExportData data;
	data.pNonce.resize(IdentifyMessage::NonceSize);
	_cryptoController->crypto()->rng().GenerateBlock((byte*)data.pNonce.data(), data.pNonce.size());
	data.partnerId = _deviceId;
	data.trusted = !password.isNull();

	QByteArray salt;
	CryptoPP::SecByteBlock key;
	std::tie(data.scheme, salt, key) = _cryptoController->generateExportKey(password);
	data.cmac = _cryptoController->createExportCmac(data.scheme, key, data.signData());

	if(includeServer)
		data.config = QSharedPointer<RemoteConfig>::create(loadConfig());

	_exportsCache.insert(data.pNonce, key);
	return std::tuple<ExportData, QByteArray, CryptoPP::SecByteBlock>{data, salt, key};
}

bool RemoteConnector::isSyncEnabled() const
{
	return sValue(keyRemoteEnabled).toBool();
}

QString RemoteConnector::deviceName() const
{
	return sValue(keyDeviceName).toString();
}

void RemoteConnector::reconnect()
{
	_stateMachine->submitEvent(QStringLiteral("reconnect"));
}

void RemoteConnector::disconnect()
{
	triggerError(false);
}

void RemoteConnector::resync()
{
	if(!isIdle())
		return;
	emit remoteEvent(RemoteReadyWithChanges);
	_socket->sendBinaryMessage(serializeMessage(SyncMessage()));
}

void RemoteConnector::listDevices()
{
	if(!isIdle())
		return;
	_socket->sendBinaryMessage(serializeMessage(ListDevicesMessage()));
}

void RemoteConnector::removeDevice(const QUuid &deviceId)
{
	if(!isIdle())
		return;
	if(deviceId == _deviceId) {
		logWarning() << "Cannot delete your own device. Use reset the account instead";
		return;
	}
	_socket->sendBinaryMessage(serializeMessage<RemoveMessage>(deviceId));
}

void RemoteConnector::resetAccount(bool clearConfig)
{
	try {
		if(clearConfig) { //always clear, in order to reset imports
			settings()->remove(keyRemoteConfig);
			settings()->remove(keyImport);
		}

		auto devId = _deviceId;
		if(devId.isNull())
			devId = sValue(keyDeviceId).toUuid();

		if(!devId.isNull()) {
			_exportsCache.clear();
			settings()->remove(keyDeviceId);
			_cryptoController->deleteKeyMaterial(devId);
			if(isIdle()) {//delete yourself. Remote will disconnecte once done
				Q_ASSERT_X(_deviceId == devId, Q_FUNC_INFO, "Stored deviceid does not match the current one");
				_socket->sendBinaryMessage(serializeMessage<RemoveMessage>(devId));
			} else {
				_deviceId = QUuid();
				reconnect();
			}
		} else {
			logInfo() << "Skipping server reset, not registered to a server";
			//still reconnect, as this "completes" the operation (and is needed for imports)
			reconnect();
		}
	} catch(Exception &e) {
		logCritical() << "Failed to reset account completly:" << e.what();
		triggerError(true);
	}
}

void RemoteConnector::prepareImport(const ExportData &data)
{
	//assume data was already "validated"
	if(data.config)
		storeConfig(*(data.config));
	else
		settings()->remove(keyRemoteConfig);
	settings()->setValue(keyImportTrusted, data.trusted);
	settings()->setValue(keyImportNonce, data.pNonce);
	settings()->setValue(keyImportPartner, data.partnerId);
	settings()->setValue(keyImportScheme, data.scheme);
	settings()->setValue(keyImportCmac, data.cmac);
	//after storing, continue with "normal" reset. This MUST be done by the engine, thus not in this function
}

void RemoteConnector::loginReply(const QUuid &deviceId, bool accept)
{

}

void RemoteConnector::uploadData(const QByteArray &key, const QByteArray &changeData)
{
	try {
		if(!isIdle()) {
			logWarning() << "Can't upload when not in idle state. Ignoring request";
			return;
		}

		ChangeMessage message(key);
		std::tie(message.keyIndex, message.salt, message.data) = _cryptoController->encrypt(changeData);
		_socket->sendBinaryMessage(serializeMessage(message));
	} catch(Exception &e) {
		logCritical() << e.what();
		triggerError(false);
	}
}

void RemoteConnector::downloadDone(const quint64 key)
{
	try {
		if(!isIdle()) {
			logWarning() << "Can't download when not in idle state. Ignoring request";
			return;
		}

		ChangedAckMessage message(key);
		_socket->sendBinaryMessage(serializeMessage(message));
		emit progressIncrement();
	} catch(Exception &e) {
		logCritical() << e.what();
		triggerError(false);
	}
}

void RemoteConnector::setSyncEnabled(bool syncEnabled)
{
	if (sValue(keyRemoteEnabled).toBool() == syncEnabled)
		return;

	settings()->setValue(keyRemoteEnabled, syncEnabled);
	reconnect();
	emit syncEnabledChanged(syncEnabled);
}

void RemoteConnector::setDeviceName(const QString &deviceName)
{
	if(sValue(keyDeviceName).toString() != deviceName) {
		settings()->setValue(keyDeviceName, deviceName);
		emit deviceNameChanged(deviceName);
		reconnect();
	}
}

void RemoteConnector::resetDeviceName()
{
	if(settings()->contains(keyDeviceName)) {
		settings()->remove(keyDeviceName);
		emit deviceNameChanged(deviceName());
		reconnect();
	}
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
		else if(isType<ChangeAckMessage>(name))
			onChangeAck(deserializeMessage<ChangeAckMessage>(stream));
		else if(isType<ChangedMessage>(name))
			onChanged(deserializeMessage<ChangedMessage>(stream));
		else if(isType<ChangedInfoMessage>(name))
			onChangedInfo(deserializeMessage<ChangedInfoMessage>(stream));
		else if(isType<LastChangedMessage>(name))
			onLastChanged(deserializeMessage<LastChangedMessage>(stream));
		else if(isType<DevicesMessage>(name))
			onDevices(deserializeMessage<DevicesMessage>(stream));
		else if(isType<RemovedMessage>(name))
			onRemoved(deserializeMessage<RemovedMessage>(stream));
		else {
			logWarning().noquote() << "Unknown message received:" << typeName(name);
			triggerError(true);
		}
	} catch(DataStreamException &e) {
		logCritical() << "Remote message error:" << e.what();
		triggerError(true);
	} catch(Exception &e) {
		logCritical() << e.what();
		triggerError(false);
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
	emit remoteEvent(RemoteConnecting);
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
		_pingTimer->setInterval(scdtime(std::chrono::minutes(tOut)));
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

	auto keys = sValue(keyHeaders).value<RemoteConfig::HeaderHash>();
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

void RemoteConnector::onEntryIdleState()
{
	_retryIndex = 0;
	if(_expectChanges) {
		_expectChanges = false;
		logDebug() << "Server has changes. Reloading states";
		remoteEvent(RemoteReadyWithChanges);
	} else
		emit remoteEvent(RemoteReady);
}

void RemoteConnector::onExitActiveState()
{
	_deviceCache.clear();
	emit remoteEvent(RemoteDisconnected);
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

	//load crypto stuff
	if(!loadIdentity()) {
		logCritical() << "Unable to load user identity. Cannot synchronize";
		return false;
	}

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

	return true;
}

bool RemoteConnector::loadIdentity()
{
	try {
		auto nId = sValue(keyDeviceId).toUuid();
		if(nId != _deviceId || nId.isNull()) { //only if new id is null or id has changed
			_deviceId = nId;
			_cryptoController->clearKeyMaterial();
			if(!_cryptoController->acquireStore(!_deviceId.isNull())) //no keystore -> can neither save nor load...
				return false;

			if(_deviceId.isNull()) //no user -> nothing to be loaded
				return true;

			_cryptoController->loadKeyMaterial(_deviceId);
		}
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

	QTimer::singleShot(scdtime(retryTimeout), this, [this](){
		if(_retryIndex != 0)
			reconnect();
	});

	return retryTimeout;
}

QVariant RemoteConnector::sValue(const QString &key) const
{
	if(key == keyHeaders) {
		if(settings()->childGroups().contains(keyHeaders)) {
			settings()->beginGroup(keyHeaders);
			auto keys = settings()->childKeys();
			RemoteConfig::HeaderHash headers;
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
		return config.url();
	else if(key == keyAccessKey)
		return config.accessKey();
	else if(key == keyHeaders)
		return QVariant::fromValue(config.headers());
	else if(key == keyKeepaliveTimeout)
		return QVariant::fromValue(config.keepaliveTimeout());
	else if(key == keyRemoteEnabled)
		return true;
	else if(key == keyDeviceName)
		return QSysInfo::machineHostName();
	else
		return {};
}

RemoteConfig RemoteConnector::loadConfig() const
{
	RemoteConfig config;
	config.setUrl(sValue(keyRemoteUrl).toUrl());
	config.setAccessKey(sValue(keyAccessKey).toString());
	config.setHeaders(sValue(keyHeaders).value<RemoteConfig::HeaderHash>());
	config.setKeepaliveTimeout(sValue(keyKeepaliveTimeout).toInt());
	return config;
}

void RemoteConnector::storeConfig(const RemoteConfig &config)
{
	//store remote config as well -> via current values, taken from defaults
	settings()->setValue(keyRemoteUrl, config.url());
	settings()->setValue(keyAccessKey, config.accessKey());
	settings()->beginGroup(keyHeaders);
	for(auto it = config.headers().begin(); it != config.headers().end(); it++)
		settings()->setValue(QString::fromUtf8(it.key()), it.value());
	settings()->endGroup();
	settings()->setValue(keyKeepaliveTimeout, config.keepaliveTimeout());
}

void RemoteConnector::onError(const ErrorMessage &message)
{
	logCritical() << message;
	triggerError(message.canRecover);

	if(!message.canRecover) {
		switch(message.type) {
		case ErrorMessage::IncompatibleVersionError:
			emit controllerError(tr("Server is not compatibel with your application version."));
			break;
		case ErrorMessage::AuthenticationError: //TODO special treatment? (or better description)
			emit controllerError(tr("Authentication failed. Try to remove and add your device again, or reset your account!"));
			break;
		case ErrorMessage::ClientError:
		case ErrorMessage::ServerError:
		case ErrorMessage::UnexpectedMessageError:
			emit controllerError(tr("Internal application error. Check the logs for details."));
			break;
		case ErrorMessage::UnknownError:
		default:
			emit controllerError(tr("Unknown error occured."));
			break;
		}
	}
}

void RemoteConnector::onIdentify(const IdentifyMessage &message)
{
	// allow connecting too, because possible event order: [Connecting] -> connected -> onIdentify -> [Connected] -> ...
	// instead of the "clean" order: [Connecting] -> connected -> [Connected] -> onIdentify -> ...
	// can happen when the message is received before the connected event has been sent
	if(!_stateMachine->isActive(QStringLiteral("Connected")) &&
	   !_stateMachine->isActive(QStringLiteral("Connecting"))) {
		logWarning() << "Unexpected IdentifyMessage";
		triggerError(true);
	} else {
		if(!_deviceId.isNull()) {
			LoginMessage msg(_deviceId,
							 sValue(keyDeviceName).toString(),
							 message.nonce);
			auto signedMsg = _cryptoController->serializeSignedMessage(msg);
			_stateMachine->submitEvent(QStringLiteral("awaitLogin"));
			_socket->sendBinaryMessage(signedMsg);
			logDebug() << "Sent login message for device id" << _deviceId;
		} else {
			_cryptoController->createPrivateKeys(message.nonce);

			//check if register or import
			auto pNonce = settings()->value(keyImportNonce).toByteArray();
			if(pNonce.isEmpty()) {
				RegisterMessage msg(sValue(keyDeviceName).toString(),
									message.nonce,
									_cryptoController->crypto()->signKey(),
									_cryptoController->crypto()->cryptKey(),
									_cryptoController->crypto());
				auto signedMsg = _cryptoController->serializeSignedMessage(msg);
				_stateMachine->submitEvent(QStringLiteral("awaitRegister"));
				_socket->sendBinaryMessage(signedMsg);
				logDebug() << "Sent registration message for new id";
			} else {
				Q_UNIMPLEMENTED();
				triggerError(false);
			}
		}
	}
}

void RemoteConnector::onAccount(const AccountMessage &message)
{
	if(!_stateMachine->isActive(QStringLiteral("Registering"))) {
		logWarning() << "Unexpected AccountMessage";
		triggerError(true);
	} else {
		_deviceId = message.deviceId;

		settings()->setValue(keyDeviceId, _deviceId);
		storeConfig(loadConfig());//make shure it's stored, in case it was from defaults

		_cryptoController->storePrivateKeys(_deviceId);
		logDebug() << "Registration successful";
		_expectChanges = false;
		_stateMachine->submitEvent(QStringLiteral("account"));
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
		_expectChanges = message.hasChanges;
		_stateMachine->submitEvent(QStringLiteral("account"));
	}
}

void RemoteConnector::onChangeAck(const ChangeAckMessage &message)
{
	if(!isIdle()) {
		logWarning() << "Unexpected DoneMessage";
		triggerError(true);
	} else
		emit uploadDone(message.dataId);
}

void RemoteConnector::onChanged(const ChangedMessage &message)
{
	if(!isIdle()) {
		logWarning() << "Unexpected ChangedMessage";
		triggerError(true);
	} else {
		auto data = _cryptoController->decrypt(message.keyIndex,
											   message.salt,
											   message.data);
		emit downloadData(message.dataIndex, data);
	}
}

void RemoteConnector::onChangedInfo(const ChangedInfoMessage &message)
{
	if(!isIdle()) {
		logWarning() << "Unexpected ChangedInfoMessage";
		triggerError(true);
	} else {
		logDebug() << "Started downloading, estimated changes:" << message.changeEstimate;
		//emit event to enter downloading state
		emit remoteEvent(RemoteReadyWithChanges);
		emit progressAdded(message.changeEstimate);
		//TODO make use of change count
		//parse as usual
		onChanged(message);
	}
}

void RemoteConnector::onLastChanged(const LastChangedMessage &message)
{
	Q_UNUSED(message)

	if(!isIdle()) {
		logWarning() << "Unexpected LastChangedMessage";
		triggerError(true);
	} else {
		logDebug() << "Completed downloading changes";
		emit remoteEvent(RemoteReady); //back to normal
	}
}

void RemoteConnector::onDevices(const DevicesMessage &message)
{
	if(!isIdle()) {
		logWarning() << "Unexpected DevicesMessage";
		triggerError(true);
	} else {
		logDebug() << "Received list of devices with" << message.devices.size() << "entries";
		_deviceCache.clear();
		foreach(auto device, message.devices)
			_deviceCache.append(DeviceInfo{std::get<0>(device), std::get<1>(device), std::get<2>(device)});
		emit devicesListed(_deviceCache);
	}
}

void RemoteConnector::onRemoved(const RemovedMessage &message)
{
	if(!isIdle()) {
		logWarning() << "Unexpected DevicesMessage";
		triggerError(true);
	} else {
		logDebug() << "Device with id" << message.deviceId << "was removed";
		if(_deviceId == message.deviceId) {
			_deviceId = QUuid();
			reconnect();
		} else {
			//in case the device was known, remove it
			for(auto it = _deviceCache.begin(); it != _deviceCache.end(); it++) {
				if(it->deviceId() == message.deviceId) {
					_deviceCache.erase(it);
					emit devicesListed(_deviceCache);
					break;
				}
			}
		}
	}
}



ExportData::ExportData() :
	trusted(false),
	pNonce(),
	partnerId(),
	scheme(),
	cmac(),
	config(nullptr)
{}

QByteArray ExportData::signData() const
{
	return pNonce +
			partnerId.toRfc4122() +
			scheme;
}
