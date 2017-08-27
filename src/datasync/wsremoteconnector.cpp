#include "defaults.h"
#include "wsauthenticator.h"
#include "wsremoteconnector_p.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtCore/QTimerEvent>

using namespace QtDataSync;

#define LOG defaults()->loggingCategory()

const QString WsRemoteConnector::keyRemoteEnabled(QStringLiteral("RemoteConnector/remoteEnabled"));
const QString WsRemoteConnector::keyRemoteUrl(QStringLiteral("RemoteConnector/remoteUrl"));
const QString WsRemoteConnector::keyHeadersGroup(QStringLiteral("RemoteConnector/headers"));
const QString WsRemoteConnector::keyVerifyPeer(QStringLiteral("RemoteConnector/verifyPeer"));
const QString WsRemoteConnector::keyUserIdentity(QStringLiteral("RemoteConnector/userIdentity"));
const QString WsRemoteConnector::keySharedSecret(QStringLiteral("RemoteConnector/sharedSecret"));
const QString WsRemoteConnector::keyResync(QStringLiteral("RemoteConnector/resync"));
const QVector<int> WsRemoteConnector::timeouts = {5 * 1000, 10 * 1000, 30 * 1000, 60 * 1000, 5 * 60 * 1000, 10 * 60 * 1000};

WsRemoteConnector::WsRemoteConnector(QObject *parent) :
	RemoteConnector(parent),
	socket(nullptr),
	settings(nullptr),
	state(Disconnected),
	retryIndex(0),
	needResync(false),
	operationTimer(new QTimer(this)),
	pingTimerId(-1),
	decryptReply(false),
	currentKey(),
	currentKeyProperty()
{}

void WsRemoteConnector::initialize(Defaults *defaults, Encryptor *cryptor)
{
	RemoteConnector::initialize(defaults, cryptor);
	settings = defaults->createSettings(this);

	needResync = settings->value(WsRemoteConnector::keyResync, needResync).toBool();

	operationTimer->setInterval(30000);//30 secs timout
	operationTimer->setSingleShot(true);
	connect(operationTimer, &QTimer::timeout,
			this, &WsRemoteConnector::operationTimeout);
	pingTimerId = startTimer(3 * 60 * 1000, Qt::VeryCoarseTimer);//ping every 3 minutes to keep alive

	reconnect();
}

void WsRemoteConnector::finalize()
{
	settings->setValue(WsRemoteConnector::keyResync, needResync);
	settings->sync();
	RemoteConnector::finalize();
}

bool WsRemoteConnector::isSyncEnabled() const
{
	QScopedPointer<QSettings> localSettings(defaults()->createSettings());
	return localSettings->value(WsRemoteConnector::keyRemoteEnabled, true).toBool();
}

bool WsRemoteConnector::setSyncEnabled(bool syncEnabled)
{
	if(syncEnabled != settings->value(WsRemoteConnector::keyRemoteEnabled, true).toBool()) {
		settings->setValue(WsRemoteConnector::keyRemoteEnabled, syncEnabled);
		settings->sync();
		reconnect();
		return true;
	} else
		return false;
}

Authenticator *WsRemoteConnector::createAuthenticator(Defaults *defaults, QObject *parent)
{
	return new WsAuthenticator(this, defaults, parent);
}

void WsRemoteConnector::reconnect()
{
	if(state == Connecting ||
	   state == Closing)
		return;

	if(socket) {
		state = Closing;
		connect(socket, &QWebSocket::destroyed,
				this, &WsRemoteConnector::reconnect,
				Qt::QueuedConnection);
		socket->close();
	} else {
		emit clearAuthenticationError();
		state = Connecting;
		settings->sync();
		emit remoteStateChanged(RemoteConnecting);

		auto remoteUrl = settings->value(keyRemoteUrl).toUrl();
		if(!remoteUrl.isValid() || !settings->value(keyRemoteEnabled, true).toBool()) {
			state = Disconnected;
			emit remoteStateChanged(RemoteDisconnected);
			return;
		}

		socket = new QWebSocket(settings->value(keySharedSecret, QStringLiteral("QtDataSync")).toString(),
								QWebSocketProtocol::VersionLatest,
								this);

		if(!settings->value(keyVerifyPeer, true).toBool()) {
			auto conf = socket->sslConfiguration();
			conf.setPeerVerifyMode(QSslSocket::VerifyNone);
			socket->setSslConfiguration(conf);
		}

		connect(socket, &QWebSocket::connected,
				this, &WsRemoteConnector::connected);
		connect(socket, &QWebSocket::binaryMessageReceived,
				this, &WsRemoteConnector::binaryMessageReceived);
		connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
				this, &WsRemoteConnector::error);
		connect(socket, &QWebSocket::sslErrors,
				this, &WsRemoteConnector::sslErrors);
		connect(socket, &QWebSocket::disconnected,
				this, &WsRemoteConnector::disconnected,
				Qt::QueuedConnection);

		QNetworkRequest request(remoteUrl);
		request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
		request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
		request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
		request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);

		settings->beginGroup(keyHeadersGroup);
		auto keys = settings->childKeys();
		foreach(auto key, keys)
			request.setRawHeader(key.toUtf8(), settings->value(key).toByteArray());
		settings->endGroup();

		socket->open(request);
	}
}

void WsRemoteConnector::reloadRemoteState()
{
	switch (state) {
	case Disconnected:
		reconnect();
		break;
	case Idle:
		emit remoteStateChanged(RemoteLoadingState);
		if(needResync)
			emit performLocalReset(false);//direct connected, thus "inline"
		state = Reloading;
		sendCommand("loadChanges", needResync);
		needResync = false;
		break;
	case Operating:
		retry();
		break;
	default:
		break;
	}
}

void WsRemoteConnector::requestResync()
{
	needResync = true;
	reloadRemoteState();
}

void WsRemoteConnector::download(const ObjectKey &key, const QByteArray &keyProperty)
{
	if(state != Idle)
		emit operationFailed(QStringLiteral("Remote connector state does not allow downloads"));
	else {
		state = Operating;
		QJsonObject data;
		data[QStringLiteral("type")] = QString::fromUtf8(key.first);
		data[QStringLiteral("key")] = key.second;
		sendCommand("load", data);

		//if cryptor is valid, expect the reply to be encrypted
		if(cryptor()) {
			currentKey = key;
			currentKeyProperty = keyProperty;
			decryptReply = true;
		}
	}
}

void WsRemoteConnector::upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty)
{
	if(state != Idle)
		emit operationFailed(QStringLiteral("Remote connector state does not allow uploads"));
	else {
		state = Operating;
		QJsonObject data;
		data[QStringLiteral("type")] = QString::fromUtf8(key.first);
		data[QStringLiteral("key")] = key.second;
		if(cryptor()) {
			try {
				data[QStringLiteral("value")] = cryptor()->encrypt(key, object, keyProperty);
				sendCommand("save", data);
			} catch(QException &e) {
				emit operationFailed(QString::fromUtf8(e.what()));
				state = Idle;
			}
		} else {
			data[QStringLiteral("value")] = object;
			sendCommand("save", data);
		}
	}
}

void WsRemoteConnector::remove(const ObjectKey &key, const QByteArray &)
{
	if(state != Idle)
		emit operationFailed(QStringLiteral("Remote connector state does not allow removals"));
	else {
		state = Operating;
		QJsonObject data;
		data[QStringLiteral("type")] = QString::fromUtf8(key.first);
		data[QStringLiteral("key")] = key.second;
		sendCommand("remove", data);
	}
}

void WsRemoteConnector::markUnchanged(const ObjectKey &key, const QByteArray &)
{
	if(state != Idle)
		emit operationFailed(QStringLiteral("Remote connector state does not allow marking as unchanged"));
	else {
		state = Operating;
		QJsonObject data;
		data[QStringLiteral("type")] = QString::fromUtf8(key.first);
		data[QStringLiteral("key")] = key.second;
		sendCommand("markUnchanged", data);
	}

}

void WsRemoteConnector::resetUserData(const QVariant &extraData, const QByteArray &)
{
	if(socket) {
		sendCommand("deleteOldDevice");
		operationTimer->stop();//unimportant message, no timeout
	}

	if(extraData.isValid())
		settings->setValue(keyUserIdentity, extraData);
	else
		settings->remove(keyUserIdentity);
	reconnect();
}

void WsRemoteConnector::timerEvent(QTimerEvent *event)
{
	if(event->timerId() == pingTimerId) {
		event->accept();
		if(socket)
			socket->ping();
	} else
		RemoteConnector::timerEvent(event);
}

void WsRemoteConnector::connected()
{
	//reset retry
	retryIndex = 0;

	state = Identifying;
	//check if a client ID exists
	auto id = settings->value(keyUserIdentity).toByteArray();
	if(id.isNull()) {
		QJsonObject data;
		data[QStringLiteral("deviceId")] = QString::fromUtf8(getDeviceId());
		sendCommand("createIdentity", data);
	} else {
		QJsonObject data;
		data[QStringLiteral("userId")] = QString::fromUtf8(id);
		data[QStringLiteral("deviceId")] = QString::fromUtf8(getDeviceId());
		sendCommand("identify", data);
	}
}

void WsRemoteConnector::disconnected()
{
	operationTimer->stop();//just to make shure

	if(state == Operating)
		emit operationFailed(QStringLiteral("Connection closed"));

	if(state != Closing) {
		auto delta = retry();
		if(state == Connecting) {
			qCWarning(LOG) << "Unable to connect to server. Retrying in"
						   << delta / 1000
						   << "seconds";
		} else {
			qCWarning(LOG) << "Unexpected disconnect from server with exit code"
						   << socket->closeCode()
						   << ":"
						   << socket->closeReason()
						   << ". Retrying in"
						   << delta / 1000
						   << "seconds";
		}
	}

	state = Disconnected;
	socket->deleteLater();
	socket = nullptr;

	//always "disable" remote for the state changer
	emit remoteStateChanged(RemoteDisconnected);
}

void WsRemoteConnector::binaryMessageReceived(const QByteArray &message)
{
	operationTimer->stop();

	QJsonParseError error;
	auto doc = QJsonDocument::fromJson(message, &error);
	if(error.error != QJsonParseError::NoError) {
		qCWarning(LOG) << "Invalid data received. Parser error:"
					   << error.errorString();
		return;
	}

	auto obj = doc.object();
	auto data = obj[QStringLiteral("data")];
	if(obj[QStringLiteral("command")] == QStringLiteral("identified"))
		identified(data.toObject());
	else if(obj[QStringLiteral("command")] == QStringLiteral("identifyFailed"))
		identifyFailed();
	else if(obj[QStringLiteral("command")] == QStringLiteral("changeState"))
		changeState(data.toObject());
	else if(obj[QStringLiteral("command")] == QStringLiteral("notifyChanged"))
		notifyChanged(data.toObject());
	else if(obj[QStringLiteral("command")] == QStringLiteral("completed"))
		completed(data.toObject());
	else {
		qCWarning(LOG) << "Unkown command received:"
					   << obj[QStringLiteral("command")].toString();
	}
}

void WsRemoteConnector::error()
{
	qCWarning(LOG) << "Socket error"
				   << socket->errorString();
	emit remoteStateChanged(RemoteDisconnected);
	socket->close();
}

void WsRemoteConnector::sslErrors(const QList<QSslError> &errors)
{
	foreach(auto error, errors) {
		qCWarning(LOG) << "SSL errors"
					   << error.errorString();
	}

	if(settings->value(keyVerifyPeer, true).toBool()) {
		emit remoteStateChanged(RemoteDisconnected);
		socket->close();
	}
}

void WsRemoteConnector::sendCommand(const QByteArray &command, const QJsonValue &data)
{
	QJsonObject message;
	message[QStringLiteral("command")] = QString::fromUtf8(command);
	message[QStringLiteral("data")] = data;

	QJsonDocument doc(message);
	socket->sendBinaryMessage(doc.toJson(QJsonDocument::Compact));
	operationTimer->start();
}

void WsRemoteConnector::operationTimeout()
{
	qCWarning(LOG) << "Network operation timed out! Try to reconnect to server.";
	reconnect();
}

void WsRemoteConnector::identified(const QJsonObject &data)
{
	auto userId = data[QStringLiteral("userId")].toString();
	needResync = needResync || data[QStringLiteral("resync")].toBool();
	settings->setValue(keyUserIdentity, userId.toUtf8());
	qCDebug(LOG) << "Identification successful";
	state = Idle;
	reloadRemoteState();
}

void WsRemoteConnector::identifyFailed()
{
	state = Closing;
	emit authenticationFailed(QStringLiteral("User does not exist!"));
	socket->close();
}

void WsRemoteConnector::changeState(const QJsonObject &data)
{
	if(data[QStringLiteral("success")].toBool()) {
		//reset retry & need resync
		retryIndex = 0;

		StateHolder::ChangeHash changeState;
		foreach(auto value, data[QLatin1String("data")].toArray()) {
			auto obj = value.toObject();
			ObjectKey key;
			key.first = obj[QStringLiteral("type")].toString().toUtf8();
			key.second = obj[QStringLiteral("key")].toString();
			changeState.insert(key, obj[QStringLiteral("changed")].toBool() ? StateHolder::Changed : StateHolder::Deleted);
		}
		emit remoteStateChanged(RemoteReady, changeState);
	} else {
		auto delta = retry();
		qCWarning(LOG) << "Failed to load state with error:"
					   << data[QStringLiteral("error")].toString()
					   << ". Retrying in"
					   << delta / 1000
					   << "seconds";
	}
	state = Idle;
}

void WsRemoteConnector::notifyChanged(const QJsonObject &data)
{
	ObjectKey key;
	key.first = data[QStringLiteral("type")].toString().toUtf8();
	key.second = data[QStringLiteral("key")].toString();
	emit remoteDataChanged(key, data[QStringLiteral("changed")].toBool() ? StateHolder::Changed : StateHolder::Deleted);
}

void WsRemoteConnector::completed(const QJsonObject &result)
{
	if(result[QStringLiteral("success")].toBool()) {
		if(decryptReply) {
			try {
				auto data = cryptor()->decrypt(currentKey, result[QStringLiteral("data")], currentKeyProperty);
				emit operationDone(data);
			} catch(QException &e) {
				emit operationFailed(QString::fromUtf8(e.what()));
			}
			currentKey = {};
			currentKeyProperty.clear();
			decryptReply = false;
		} else
			emit operationDone(result[QStringLiteral("data")]);

	} else
		emit operationFailed(result[QStringLiteral("error")].toString());
	state = Idle;
}

int WsRemoteConnector::retry()
{
	auto retryTimeout = 0;
	if(retryIndex >= timeouts.size())
		retryTimeout = timeouts.last();
	else
		retryTimeout = timeouts[retryIndex++];

	QTimer::singleShot(retryTimeout, this, &WsRemoteConnector::reloadRemoteState);

	return retryTimeout;
}
