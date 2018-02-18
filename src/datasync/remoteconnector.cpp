#include "remoteconnector_p.h"
#include "logger.h"
#include "setup_p.h"

#include <QtCore/QSysInfo>

#include "registermessage_p.h"
#include "loginmessage_p.h"
#include "accessmessage_p.h"
#include "syncmessage_p.h"
#include "keychangemessage_p.h"

#include "connectorstatemachine.h"

#if QT_HAS_INCLUDE(<chrono>)
#define scdtime(x) x
#else
#define scdtime(x) duration_cast<milliseconds>(x).count()
#endif

using namespace QtDataSync;
using namespace std::chrono;
using std::tuple;
using std::make_tuple;
using std::tie;
using std::get;
#if CRYPTOPP_VERSION >= 600
using byte = CryptoPP::byte;
#endif

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

#define logRetry(...) (_retryIndex == 0 ? logWarning(__VA_ARGS__) : (logDebug(__VA_ARGS__) << "Repeated"))

const QString RemoteConnector::keyRemoteEnabled(QStringLiteral("enabled"));
const QString RemoteConnector::keyRemoteConfig(QStringLiteral("remote"));
const QString RemoteConnector::keyRemoteUrl(QStringLiteral("remote/url"));
const QString RemoteConnector::keyRemoteAccessKey(QStringLiteral("remote/accessKey"));
const QString RemoteConnector::keyRemoteHeaders(QStringLiteral("remote/headers"));
const QString RemoteConnector::keyRemoteKeepaliveTimeout(QStringLiteral("remote/keepaliveTimeout"));
const QString RemoteConnector::keyDeviceId(QStringLiteral("deviceId"));
const QString RemoteConnector::keyDeviceName(QStringLiteral("deviceName"));
const QString RemoteConnector::keyImport(QStringLiteral("import"));
const QString RemoteConnector::keyImportKey(QStringLiteral("import/key"));
const QString RemoteConnector::keyImportNonce(QStringLiteral("import/nonce"));
const QString RemoteConnector::keyImportPartner(QStringLiteral("import/partner"));
const QString RemoteConnector::keyImportScheme(QStringLiteral("import/scheme"));
const QString RemoteConnector::keyImportCmac(QStringLiteral("import/cmac"));
const QString RemoteConnector::keySendCmac(QStringLiteral("sendCmac"));

const QVector<seconds> RemoteConnector::Timeouts = {
	seconds(5),
	seconds(10),
	seconds(30),
	minutes(1),
	minutes(5)
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
	_deviceCache(),
	_exportsCache(),
	_activeProofs()
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
	_pingTimer->setInterval(sValue(keyRemoteKeepaliveTimeout).toInt());
	_pingTimer->setTimerType(Qt::VeryCoarseTimer);
	connect(_pingTimer, &QTimer::timeout,
			this, &RemoteConnector::ping);

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

	//special timeout
	connect(this, &RemoteConnector::specialOperationTimeout,
			this, [this]() {
		triggerError(true);
	});

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

		//timout from setup, minus a delta have a chance of beeing finished before timeout
		QTimer::singleShot(qMax<int>(1000, static_cast<int>(SetupPrivate::currentTimeout()) - 1000), this, [this](){
			if(_stateMachine->isRunning())
				_stateMachine->stop();
			if(_socket)
				_socket->close();
			emit finalized();
		});
	} else
		emit finalized();
}

tuple<ExportData, QByteArray, CryptoPP::SecByteBlock> RemoteConnector::exportAccount(bool includeServer, const QString &password)
{
	if(_deviceId.isNull())
		throw Exception(defaults(), QStringLiteral("Cannot export data without beeing registered on a server."));

	ExportData data;
	data.pNonce.resize(InitMessage::NonceSize);
	_cryptoController->rng().GenerateBlock(reinterpret_cast<byte*>(data.pNonce.data()), data.pNonce.size());
	data.partnerId = _deviceId;
	data.trusted = !password.isNull();

	QByteArray salt;
	CryptoPP::SecByteBlock key;
	tie(data.scheme, salt, key) = _cryptoController->generateExportKey(password);
	data.cmac = _cryptoController->createExportCmac(data.scheme, key, data.signData());

	if(includeServer)
		data.config = QSharedPointer<RemoteConfig>::create(loadConfig());

	_exportsCache.insert(data.pNonce, key);
	return make_tuple(data, salt, key);
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

void RemoteConnector::disconnectRemote()
{
	triggerError(false);
}

void RemoteConnector::resync()
{
	if(!isIdle()){
		logInfo() << "Cannot resync when not in idle state. Ignoring request";
		return;
	}
	emit remoteEvent(RemoteReadyWithChanges);
	sendMessage(SyncMessage());
}

void RemoteConnector::listDevices()
{
	if(!isIdle()){
		logInfo() << "Cannot list devices when not in idle state. Ignoring request";
		return;
	}
	sendMessage(ListDevicesMessage());
}

void RemoteConnector::removeDevice(const QUuid &deviceId)
{
	if(!isIdle()){
		logInfo() << "Cannot remove a device when not in idle state. Ignoring request";
		return;
	}
	if(deviceId == _deviceId) {
		logWarning() << "Cannot delete your own device. Use reset the account instead";
		return;
	}
	sendMessage(RemoveMessage{deviceId});
}

void RemoteConnector::resetAccount(bool clearConfig)
{
	if(clearConfig) { //always clear, in order to reset imports
		settings()->remove(keyRemoteConfig);
		settings()->remove(keyImport);
	}

	auto devId = _deviceId;
	if(devId.isNull())
		devId = sValue(keyDeviceId).toUuid();

	if(!devId.isNull()) {
		clearCaches(true);
		settings()->remove(keyDeviceId);
		_cryptoController->deleteKeyMaterial(devId);
		if(isIdle()) {//delete yourself. Disconnecting happens after that
			Q_ASSERT_X(_deviceId == devId, Q_FUNC_INFO, "Stored deviceid does not match the current one");
			logDebug() << "Deleting self from server";
			sendMessage(RemoveMessage{devId});
		} else {
			_deviceId = QUuid();
			logDebug() << "Account data resetted. Reconnecting to server";
			reconnect();
		}
	} else {
		logDebug() << "Skipping server reset, not registered to a server";
		//still reconnect, as this "completes" the operation (and is needed for imports)
		reconnect();
	}
}

void RemoteConnector::changeRemote(const RemoteConfig &config)
{
	storeConfig(config);
	//after storing, continue with "normal" reset. This MUST be done by the engine, thus not in this function
	logDebug() << "Prepared new remote configuration for next reconnect";
}

void RemoteConnector::prepareImport(const ExportData &data, const CryptoPP::SecByteBlock &key)
{
	//assume data was already "validated"
	if(data.config)
		storeConfig(*(data.config));
	else
		settings()->remove(keyRemoteConfig);
	settings()->setValue(keyImportNonce, data.pNonce);
	settings()->setValue(keyImportPartner, data.partnerId);
	settings()->setValue(keyImportScheme, data.scheme);
	settings()->setValue(keyImportCmac, data.cmac);
	if(data.trusted) {
		Q_ASSERT_X(!key.empty(), Q_FUNC_INFO, "Cannot have trusted data without a key");
		settings()->setValue(keyImportKey, QByteArray(reinterpret_cast<const char*>(key.data()), static_cast<int>(key.size())));
	} else
		settings()->remove(keyImportKey);
	//after storing, continue with "normal" reset. This MUST be done by the engine, thus not in this function
	logDebug() << "Imported account data and prepared it for next reconnect";
}

void RemoteConnector::loginReply(const QUuid &deviceId, bool accept)
{
	if(!isIdle()) {
		logWarning() << "Can't react to login when not in idle state. Ignoring request";
		return;
	}

	try {
		auto crypto = _activeProofs.take(deviceId);
		if(!crypto) {
			logWarning() << "Received login reply for non existant request. Propably already handeled";
			return;
		}

		if(accept) {
			AcceptMessage message(deviceId);
			tie(message.index, message.scheme, message.secret) = _cryptoController->encryptSecretKey(crypto.data(), crypto->encryptionKey());
			sendSignedMessage(message);
			logDebug() << "Granting access to account for device" << deviceId;
		} else {
			sendMessage(DenyMessage{deviceId});
			logInfo() << "Rejected access to account for device" << deviceId;
		}
	} catch(Exception &e) {
		logWarning() << "Failed to reply to login with error:" << e.what();
		//simply send a deny
		sendMessage(DenyMessage{deviceId});
	}
}

void RemoteConnector::initKeyUpdate()
{
	if(!isIdle()) {
		logWarning() << "Can't update exchange key when not in idle state. Ignoring request";
		return;
	}

	try {
		logDebug() << "Initializing exchange key update";
		sendMessage(KeyChangeMessage{_cryptoController->keyIndex() + 1});
	} catch(Exception &e) {
		onError({ErrorMessage::ClientError, e.qWhat()}, Message::messageName<KeyChangeMessage>());
	}
}

void RemoteConnector::uploadData(const QByteArray &key, const QByteArray &changeData)
{
	if(!isIdle()) {
		logWarning() << "Can't upload when not in idle state. Ignoring request";
		return;
	}

	try {
		ChangeMessage message(key);
		tie(message.keyIndex, message.salt, message.data) = _cryptoController->encryptData(changeData);
		sendMessage(message);
	} catch(Exception &e) {
		onError({ErrorMessage::ClientError, e.qWhat()}, Message::messageName<ChangeMessage>());
	}
}

void RemoteConnector::uploadDeviceData(const QByteArray &key, const QUuid &deviceId, const QByteArray &changeData)
{
	if(!isIdle()) {
		logWarning() << "Can't upload when not in idle state. Ignoring request";
		return;
	}

	try {
		DeviceChangeMessage message(key, deviceId);
		tie(message.keyIndex, message.salt, message.data) = _cryptoController->encryptData(changeData);
		sendMessage(message);
	} catch(Exception &e) {
		onError({ErrorMessage::ClientError, e.qWhat()}, Message::messageName<DeviceChangeMessage>());
	}
}

void RemoteConnector::downloadDone(const quint64 key)
{
	if(!isIdle()) {
		logWarning() << "Can't download when not in idle state. Ignoring request";
		return;
	}

	try {
		ChangedAckMessage message(key);
		sendMessage(message);
		emit progressIncrement();
		beginOp(minutes(5), false);
	} catch(Exception &e) {
		onError({ErrorMessage::ClientError, e.qWhat()}, Message::messageName<ChangedAckMessage>());
	}
}

void RemoteConnector::setSyncEnabled(bool syncEnabled)
{
	if (sValue(keyRemoteEnabled).toBool() == syncEnabled)
		return;

	settings()->setValue(keyRemoteEnabled, syncEnabled);
	if(syncEnabled)
		reconnect();
	else
		disconnectRemote();
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
	endOp();
	logDebug() << "Successfully connected to remote server";
	_stateMachine->submitEvent(QStringLiteral("connected"));
}

void RemoteConnector::disconnected()
{
	endOp(); //to be safe
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
	if(message == Message::PingMessage) {
		_awaitingPing = false;
		_pingTimer->start();
		return;
	}

	QByteArray name;
	try {
		QDataStream stream(message);
		Message::setupStream(stream);
		stream.startTransaction();
		stream >> name;
		if(!stream.commitTransaction())
			throw DataStreamException(stream);

		if(Message::isType<ErrorMessage>(name))
			onError(Message::deserializeMessage<ErrorMessage>(stream));
		else if(Message::isType<IdentifyMessage>(name))
			onIdentify(Message::deserializeMessage<IdentifyMessage>(stream));
		else if(Message::isType<AccountMessage>(name))
			onAccount(Message::deserializeMessage<AccountMessage>(stream));
		else if(Message::isType<WelcomeMessage>(name))
			onWelcome(Message::deserializeMessage<WelcomeMessage>(stream));
		else if(Message::isType<GrantMessage>(name))
			onGrant(Message::deserializeMessage<GrantMessage>(stream));
		else if(Message::isType<ChangeAckMessage>(name))
			onChangeAck(Message::deserializeMessage<ChangeAckMessage>(stream));
		else if(Message::isType<DeviceChangeAckMessage>(name))
			onDeviceChangeAck(Message::deserializeMessage<DeviceChangeAckMessage>(stream));
		else if(Message::isType<ChangedMessage>(name))
			onChanged(Message::deserializeMessage<ChangedMessage>(stream));
		else if(Message::isType<ChangedInfoMessage>(name))
			onChangedInfo(Message::deserializeMessage<ChangedInfoMessage>(stream));
		else if(Message::isType<LastChangedMessage>(name))
			onLastChanged(Message::deserializeMessage<LastChangedMessage>(stream));
		else if(Message::isType<DevicesMessage>(name))
			onDevices(Message::deserializeMessage<DevicesMessage>(stream));
		else if(Message::isType<RemoveAckMessage>(name))
			onRemoveAck(Message::deserializeMessage<RemoveAckMessage>(stream));
		else if(Message::isType<ProofMessage>(name))
			onProof(Message::deserializeMessage<ProofMessage>(stream));
		else if(Message::isType<AcceptAckMessage>(name))
			onAcceptAck(Message::deserializeMessage<AcceptAckMessage>(stream));
		else if(Message::isType<MacUpdateAckMessage>(name))
			onMacUpdateAck(Message::deserializeMessage<MacUpdateAckMessage>(stream));
		else if(Message::isType<DeviceKeysMessage>(name))
			onDeviceKeys(Message::deserializeMessage<DeviceKeysMessage>(stream));
		else if(Message::isType<NewKeyAckMessage>(name))
			onNewKeyAck(Message::deserializeMessage<NewKeyAckMessage>(stream));
		else {
			logWarning().noquote() << "Unknown message received:" << Message::typeName(name);
			triggerError(true);
		}
	} catch(DataStreamException &e) {
		onError({ErrorMessage::IncompatibleVersionError, QString::fromUtf8(e.what())}, "invalid remote message");
	} catch(IncompatibleVersionException &e) {
		onError({ErrorMessage::IncompatibleVersionError, QString::fromUtf8(e.what())}, "server version not accepted");
	} catch(Exception &e) {
		//simulate a "normal" client error
		onError({ErrorMessage::ClientError, e.qWhat()}, name);
#ifdef __clang__
	} catch(std::exception &e) {
#else
	} catch(CryptoPP::Exception &e) {
#endif
		//simulate a "normal" client error
		CryptoException tmpExcept(defaults(), QStringLiteral("Crypto-Operation in external context failed"), e);
		onError({ErrorMessage::ClientError, tmpExcept.qWhat()}, name);
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
	for(auto error : errors) {
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
		logDebug() << "Server connection idle (ping timeout). Reconnecting to server";
		reconnect();
	} else {
		_awaitingPing = true;
		_socket->sendBinaryMessage(Message::PingMessage);
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
	_socket = new QWebSocket(sValue(keyRemoteAccessKey).toString(),
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
	auto tOut = sValue(keyRemoteKeepaliveTimeout).toInt();
	if(tOut > 0) {
		_pingTimer->setInterval(scdtime(minutes(tOut)));
		_awaitingPing = false;
		connect(_socket, &QWebSocket::connected,
				_pingTimer, QOverload<>::of(&QTimer::start));
		connect(_socket, &QWebSocket::disconnected,
				_pingTimer, &QTimer::stop);
		logDebug() << "Keepalive ping interval set to" << tOut << "minutes";
	} else
		logDebug() << "Keepalive ping disabled";

	QNetworkRequest request(remoteUrl);
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
	request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
	request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
	request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);

	auto keys = sValue(keyRemoteHeaders).value<RemoteConfig::HeaderHash>();
	for(auto it = keys.begin(); it != keys.end(); it++)
		request.setRawHeader(it.key(), it.value());

	beginSpecialOp(minutes(1)); //wait at most 1 minute for the connection
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
			beginSpecialOp(minutes(1)); //wait at most 1 minute for the disconnect
			_socket->close();
			break;
		case QAbstractSocket::BoundState:
		case QAbstractSocket::ListeningState:
			logFatal("Reached impossible client socket state - aborting");
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
			   << duration_cast<seconds>(delta).count()
			   << "seconds";
}

void RemoteConnector::onEntryIdleState()
{
	_retryIndex = 0;
	if(_cryptoController->hasKeyUpdate())
		initKeyUpdate();

	if(_expectChanges) {
		_expectChanges = false;
		logDebug() << "Server has changes. Reloading states";
		remoteEvent(RemoteReadyWithChanges);
	} else
		emit remoteEvent(RemoteReady);
}

void RemoteConnector::onExitActiveState()
{
	clearCaches(false);
	endOp(); //disconnected -> whatever operation was going on is now done
	emit remoteEvent(RemoteDisconnected);
}

void RemoteConnector::sendMessage(const Message &message)
{
	_socket->sendBinaryMessage(message.serialize());
}

void RemoteConnector::sendSignedMessage(const Message &message)
{
	_socket->sendBinaryMessage(_cryptoController->serializeSignedMessage(message));
}

bool RemoteConnector::isIdle() const
{
	return _stateMachine->isActive(QStringLiteral("Idle"));
}

bool RemoteConnector::checkIdle(const Message &message)
{
	if(isIdle())
		return true;
	else {
		logWarning().noquote() << "Unexpected" << message.typeName();
		triggerError(true);
		return false;
	}
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
			_cryptoController->acquireStore(!_deviceId.isNull());

			if(_deviceId.isNull()) //no user -> nothing to be loaded
				return true;

			_cryptoController->loadKeyMaterial(_deviceId);
		}
		return true;
	} catch(Exception &e) {
		logCritical() << "Failed to load identity with error:"
					  << e.what();
		emit controllerError(tr("Failed to load user identity! Make shure your keystore is available."));
		return false;
	}
}

void RemoteConnector::tryClose()
{
	// do not set _disconnecting, because this is unexpected
	if(_socket && _socket->state() == QAbstractSocket::ConnectedState)
		_socket->close();
}

seconds RemoteConnector::retry()
{
	seconds retryTimeout;
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

void RemoteConnector::clearCaches(bool includeExport)
{
	_deviceCache.clear();
	if(includeExport)
		_exportsCache.clear();
	_activeProofs.clear();
}

QVariant RemoteConnector::sValue(const QString &key) const
{
	if(key == keyRemoteHeaders) {
		if(settings()->childGroups().contains(keyRemoteHeaders)) {
			settings()->beginGroup(keyRemoteHeaders);
			RemoteConfig::HeaderHash headers;
			for(auto key : settings()->childKeys())
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
	else if(key == keyRemoteAccessKey)
		return config.accessKey();
	else if(key == keyRemoteHeaders)
		return QVariant::fromValue(config.headers());
	else if(key == keyRemoteKeepaliveTimeout)
		return QVariant::fromValue(config.keepaliveTimeout());
	else if(key == keyRemoteEnabled)
		return true;
	else if(key == keyDeviceName)
		return QSysInfo::machineHostName();
	else if(key == keySendCmac)
		return false;
	else
		return {};
}

RemoteConfig RemoteConnector::loadConfig() const
{
	RemoteConfig config;
	config.setUrl(sValue(keyRemoteUrl).toUrl());
	config.setAccessKey(sValue(keyRemoteAccessKey).toString());
	config.setHeaders(sValue(keyRemoteHeaders).value<RemoteConfig::HeaderHash>());
	config.setKeepaliveTimeout(sValue(keyRemoteKeepaliveTimeout).toInt());
	return config;
}

void RemoteConnector::storeConfig(const RemoteConfig &config)
{
	//clean the old config
	settings()->remove(keyRemoteConfig);
	//store remote config as well -> via current values, taken from defaults
	settings()->setValue(keyRemoteUrl, config.url());
	settings()->setValue(keyRemoteAccessKey, config.accessKey());
	settings()->beginGroup(keyRemoteHeaders);
	auto headers = config.headers();
	for(auto it = headers.constBegin(); it != headers.constEnd(); it++)
		settings()->setValue(QString::fromUtf8(it.key()), it.value());
	settings()->endGroup();
	settings()->setValue(keyRemoteKeepaliveTimeout, config.keepaliveTimeout());
}

void RemoteConnector::sendKeyUpdate()
{
	settings()->setValue(keySendCmac, true);
	auto cmac = _cryptoController->generateEncryptionKeyCmac();
	sendMessage(MacUpdateMessage{_cryptoController->keyIndex(), cmac});
	logDebug() << "Sent exchange mac for key with index" << _cryptoController->keyIndex();
}

void RemoteConnector::onError(const ErrorMessage &message, const QByteArray &messageName)
{
	if(!messageName.isEmpty())
		logCritical().noquote() << "Local error on " << messageName << ": " << message.message;
	else
		logCritical() << message;
	triggerError(message.canRecover);

	if(!message.canRecover) {
		switch(message.type) {
		case ErrorMessage::IncompatibleVersionError:
			emit controllerError(tr("Server is not compatibel with your application version."));
			break;
		case ErrorMessage::AuthenticationError:
			emit controllerError(tr("Authentication failed. Try to remove and add your device again, or reset your account!"));
			break;
		case ErrorMessage::AccessError:
			emit controllerError(tr("Account access (import) failed. The partner device was not available or did not accept your request!"));
			break;
		case ErrorMessage::KeyIndexError:
			emit controllerError(tr("Cannot update key! This client is not using the latest existing keys."));
			break;
		case ErrorMessage::KeyPendingError:
			emit controllerError(tr("Cannot update key! At least one client did not receive the previous key update."));
			break;
		case ErrorMessage::ClientError:
		case ErrorMessage::ServerError:
		case ErrorMessage::UnexpectedMessageError:
			emit controllerError(tr("Internal application error. Check the logs for details."));
			break;
		case ErrorMessage::QuotaHitError:
			emit controllerError(tr("Data quota hit. You need to synchronize changes to other devices before you can upload more changes."));
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
		emit updateUploadLimit(message.uploadLimit);
		if(!_deviceId.isNull()) {
			LoginMessage msg(_deviceId,
							 sValue(keyDeviceName).toString(),
							 message.nonce);
			sendSignedMessage(msg);
			_stateMachine->submitEvent(QStringLiteral("awaitLogin"));
			logDebug() << "Sent login message for device id" << _deviceId;
		} else {
			_cryptoController->createPrivateKeys(message.nonce);
			auto crypto = _cryptoController->crypto();

			//check if register or import
			auto pNonce = settings()->value(keyImportNonce).toByteArray();
			if(pNonce.isEmpty()) {
				RegisterMessage msg(sValue(keyDeviceName).toString(),
									message.nonce,
									crypto->signKey(),
									crypto->cryptKey(),
									crypto,
									_cryptoController->generateEncryptionKeyCmac());
				sendSignedMessage(msg);
				_stateMachine->submitEvent(QStringLiteral("awaitRegister"));
				logDebug() << "Sent registration message for new id";
			} else {
				//calc trustmac
				QByteArray trustmac;
				auto scheme = settings()->value(keyImportScheme).toByteArray();
				auto key = settings()->value(keyImportKey).toByteArray();
				if(!key.isEmpty()) {
					CryptoPP::SecByteBlock secBlock(reinterpret_cast<const byte*>(key.constData()), key.size());
					trustmac = _cryptoController->createExportCmacForCrypto(scheme, secBlock);
				}

				//send message
				AccessMessage msg(sValue(keyDeviceName).toString(),
								  message.nonce,
								  crypto->signKey(),
								  crypto->cryptKey(),
								  crypto,
								  settings()->value(keyImportNonce).toByteArray(),
								  settings()->value(keyImportPartner).toUuid(),
								  scheme,
								  settings()->value(keyImportCmac).toByteArray(),
								  trustmac);
				sendSignedMessage(msg);
				_stateMachine->submitEvent(QStringLiteral("awaitGranted"));
				logDebug() << "Sent access message for new id";
			}
		}
	}
}

void RemoteConnector::onAccount(const AccountMessage &message, bool checkState)
{
	if(checkState && !_stateMachine->isActive(QStringLiteral("Registering"))) {
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

		auto keyUpdated = false;
		if(message.hasKeyUpdate()) { //are orderd by index
			//verify the new key by using the current key
			_cryptoController->verifyCmac(_cryptoController->keyIndex(), //the key before this one
										  message.signatureData(_deviceId),
										  message.cmac);
			//import the key and set active if newer
			_cryptoController->decryptSecretKey(message.keyIndex,
												message.scheme,
												message.key,
												false);
			keyUpdated = true;
		}

		if(keyUpdated || sValue(keySendCmac).toBool())
			sendKeyUpdate();
	}
}

void RemoteConnector::onGrant(const GrantMessage &message)
{
	if(!_stateMachine->isActive(QStringLiteral("Granting"))) {
		logWarning() << "Unexpected GrantMessage";
		triggerError(true);
	} else {
		logDebug() << "Account access granted";
		_cryptoController->decryptSecretKey(message.index, message.scheme, message.secret, true);
		onAccount(message, false);
		settings()->remove(keyImport); //import succeeded, so remove import related stuff
		//update the server cmac
		sendKeyUpdate();
		emit importCompleted();
	}
}

void RemoteConnector::onChangeAck(const ChangeAckMessage &message)
{
	if(checkIdle(message))
		emit uploadDone(message.dataId);
}

void RemoteConnector::onDeviceChangeAck(const DeviceChangeAckMessage &message)
{
	if(checkIdle(message))
		emit deviceUploadDone(message.dataId, message.deviceId);
}

void RemoteConnector::onChanged(const ChangedMessage &message)
{
	if(checkIdle(message)) {
		auto data = _cryptoController->decryptData(message.keyIndex,
												   message.salt,
												   message.data);
		beginOp();//start download timeout
		emit downloadData(message.dataIndex, data);
	}
}

void RemoteConnector::onChangedInfo(const ChangedInfoMessage &message)
{
	if(checkIdle(message)) {
		logDebug() << "Started downloading, estimated changes:" << message.changeEstimate;
		//emit event to enter downloading state
		emit remoteEvent(RemoteReadyWithChanges);
		emit progressAdded(message.changeEstimate);
		//parse as usual
		onChanged(message);
	}
}

void RemoteConnector::onLastChanged(const LastChangedMessage &message)
{
	Q_UNUSED(message)

	if(checkIdle(message)) {
		logDebug() << "Completed downloading changes";
		endOp(); //downloads done
		emit remoteEvent(RemoteReady); //back to normal
	}
}

void RemoteConnector::onDevices(const DevicesMessage &message)
{
	if(checkIdle(message)) {
		logDebug() << "Received list of devices with" << message.devices.size() << "entries";
		_deviceCache.clear();
		for(auto device : message.devices)
			_deviceCache.append(DeviceInfo{get<0>(device), get<1>(device), get<2>(device)});
		emit devicesListed(_deviceCache);
	}
}

void RemoteConnector::onRemoveAck(const RemoveAckMessage &message)
{
	if(checkIdle(message)) {
		if(_deviceId == message.deviceId) {
			logDebug() << "Own device remove from server. Account reset completed. Reconnecting to server";
			_deviceId = QUuid();
			reconnect();
		} else {
			logInfo() << "Device with id" << message.deviceId << "was removed from account";
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

void RemoteConnector::onProof(const ProofMessage &message)
{
	if(checkIdle(message)) {
		try {
			//check if expected and verify cmac
			auto key = _exportsCache.take(message.pNonce);
			if(key.empty())
				throw Exception(defaults(), QStringLiteral("ProofMessage for non existing export"));
			QByteArray macData = message.pNonce +
								 _deviceId.toRfc4122() +
								 message.macscheme;
			_cryptoController->verifyImportCmac(message.macscheme, key, macData, message.cmac);

			//read (and verify) the keys
			auto cryptInfo = QSharedPointer<AsymmetricCryptoInfo>::create(_cryptoController->rng(),
																		  message.signAlgorithm,
																		  message.signKey,
																		  message.cryptAlgorithm,
																		  message.cryptKey);

			//verify trustmac if given
			auto trusted = !message.trustmac.isNull();
			if(trusted)
				_cryptoController->verifyImportCmacForCrypto(message.macscheme, key, cryptInfo.data(), message.trustmac);

			//all verifications accepted
			_activeProofs.insert(message.deviceId, cryptInfo);
			if(trusted) {//trusted -> ready to go, send back the accept
				logInfo() << "Accepted trusted import proof request for device" << message.deviceId;
				loginReply(message.deviceId, true);
			} else { //untrusted -> cache the request and signale that the login must be checked
				logInfo() << "Received untrusted import proof request for device" << message.deviceId;
				DeviceInfo info(message.deviceId, message.deviceName, cryptInfo->ownFingerprint());
				emit loginRequested(info);
			}

			//simple timer to clean up any unhandelt proof request
			auto deviceId = message.deviceId;
			QTimer::singleShot(scdtime(minutes(10)), Qt::VeryCoarseTimer, this, [this, deviceId]() {
				if(_activeProofs.remove(deviceId) > 0) {
					logInfo() << "Rejecting ProofMessage after timeout";
					sendMessage(DenyMessage{deviceId});
				}
			});
		} catch(std::exception &e) {
			logWarning() << "Rejecting ProofMessage with error:" << e.what();
			sendMessage(DenyMessage{message.deviceId});
		}
	}
}

void RemoteConnector::onAcceptAck(const AcceptAckMessage &message)
{
	if(checkIdle(message)) {
		emit accountAccessGranted(message.deviceId);
		logInfo() << "Granted access to account for device" << message.deviceId;
	}
}

void RemoteConnector::onMacUpdateAck(const MacUpdateAckMessage &message)
{
	if(checkIdle(message)) {
		settings()->remove(keySendCmac);
		logDebug() << "Mac change acknowledged";
	}
}

void RemoteConnector::onDeviceKeys(const DeviceKeysMessage &message)
{
	if(checkIdle(message)) {
		if(message.duplicated) {
			logInfo() << "Exchange key already updated (duplicate). Adjusting local state";
			_cryptoController->activateNextKey(message.keyIndex);
		} else {
			NewKeyMessage reply;
			tie(reply.keyIndex, reply.scheme) = _cryptoController->generateNextKey();
			reply.cmac = _cryptoController->generateEncryptionKeyCmac(reply.keyIndex); //cmac for the new key
			//do not store this mac to be send again!

			for(auto info : message.devices) {
				try {
					//verify the device knows the previous secret (which is still the current one)
					auto cryptInfo = QSharedPointer<AsymmetricCryptoInfo>::create(_cryptoController->rng(),
																				  get<1>(info),
																				  get<2>(info));
					_cryptoController->verifyEncryptionKeyCmac(cryptInfo.data(),
															   cryptInfo->encryptionKey(),
															   get<3>(info));

					//encrypt the secret key and send the message
					NewKeyMessage::KeyUpdate keyUpdate {
						get<0>(info),
						_cryptoController->encryptSecretKey(reply.keyIndex, cryptInfo.data(), cryptInfo->encryptionKey()),
						QByteArray()
					};
					get<2>(keyUpdate) = _cryptoController->createCmac(reply.signatureData(keyUpdate)); //uses the "old" current key
					reply.deviceKeys.append(keyUpdate);
					logDebug() << "Prepared key update for device" << get<0>(info);
				} catch(std::exception &e) {
					logWarning() << "Failed to send update exchange key to device"
								 << get<0>(info)
								 << "- device is going to be excluded from synchronisation. Error:"
								 << e.what();
				}
			}

			sendSignedMessage(reply);
			logDebug() << "Sent exchange key update to server";
		}
	}
}

void RemoteConnector::onNewKeyAck(const NewKeyAckMessage &message)
{
	if(checkIdle(message)) {
		logInfo() << "Exchange key updated accepted. Setting active key to index" << message.keyIndex;
		_cryptoController->activateNextKey(message.keyIndex);
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
