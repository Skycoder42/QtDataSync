#include "defaults.h"
#include "wsauthenticator.h"
#include "wsremoteconnector.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
using namespace QtDataSync;

#define LOG defaults()->loggingCategory()

const QString WsRemoteConnector::keyRemoteUrl(QStringLiteral("RemoteConnector/remoteUrl"));
const QString WsRemoteConnector::keyHeadersGroup(QStringLiteral("RemoteConnector/headers"));
const QString WsRemoteConnector::keyVerifyPeer(QStringLiteral("RemoteConnector/verifyPeer"));
const QString WsRemoteConnector::keyUserIdentity(QStringLiteral("RemoteConnector/userIdentity"));
const QVector<int> WsRemoteConnector::timeouts = {5 * 1000, 10 * 1000, 30 * 1000, 60 * 1000, 5 * 60 * 1000, 10 * 60 * 1000};

WsRemoteConnector::WsRemoteConnector(QObject *parent) :
	RemoteConnector(parent),
	socket(nullptr),
	settings(nullptr),
	state(Disconnected),
	retryIndex(0)
{}

void WsRemoteConnector::initialize(Defaults *defaults)
{
	RemoteConnector::initialize(defaults);
	settings = defaults->createSettings(this);
	reconnect();
}

void WsRemoteConnector::finalize()
{
	RemoteConnector::finalize();
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
		state = Connecting;
		settings->sync();

		auto remoteUrl = settings->value(keyRemoteUrl).toUrl();
		if(!remoteUrl.isValid()) {
			state = Disconnected;
			return;
		}

		socket = new QWebSocket(QStringLiteral("QtDataSync"),
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
		request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
		request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);
		request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

		settings->beginGroup(keyHeadersGroup);
		auto keys = settings->childKeys();
		foreach(auto key, keys)
			request.setRawHeader(key.toUtf8(), settings->value(key).toByteArray());
		settings->endGroup();

		socket->open(request);
	}
}

void WsRemoteConnector::reload()
{
	if(state == Idle) {
		state = Reloading;
		sendCommand("loadChanges", QJsonValue::Null);
	} else
		retry(false);
}

void WsRemoteConnector::download(const ObjectKey &key, const QByteArray &keyProperty)
{
	if(state != Idle)
		emit operationFailed("Remote connector state does not allow donwloads");
	else {
		state = Operating;
		QJsonObject data;
		data["type"] = QString::fromLatin1(key.first);
		data["key"] = key.second;
		data["keyProperty"] = QString::fromLatin1(keyProperty);
		sendCommand("load", data);
	}
}

void WsRemoteConnector::upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty)
{
	if(state != Idle)
		emit operationFailed("Remote connector state does not allow uploads");
	else {
		state = Operating;
		QJsonObject data;
		data["type"] = QString::fromLatin1(key.first);
		data["key"] = key.second;
		data["keyProperty"] = QString::fromLatin1(keyProperty);
		data["value"] = object;
		sendCommand("save", data);
	}
}

void WsRemoteConnector::remove(const ObjectKey &key, const QByteArray &keyProperty)
{
	if(state != Idle)
		emit operationFailed("Remote connector state does not allow removals");
	else {
		state = Operating;
		QJsonObject data;
		data["type"] = QString::fromLatin1(key.first);
		data["key"] = key.second;
		data["keyProperty"] = QString::fromLatin1(keyProperty);
		sendCommand("remove", data);
	}
}

void WsRemoteConnector::markUnchanged(const ObjectKey &key, const QByteArray &keyProperty)
{
	if(state != Idle)
		emit operationFailed("Remote connector state does not allow marking as unchanged");
	else {
		state = Operating;
		QJsonObject data;
		data["type"] = QString::fromLatin1(key.first);
		data["key"] = key.second;
		data["keyProperty"] = QString::fromLatin1(keyProperty);
		sendCommand("markUnchanged", data);
	}

}

void WsRemoteConnector::resetDeviceId()
{
	RemoteConnector::resetDeviceId();
	reconnect();
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
		data["deviceId"] = QString::fromUtf8(loadDeviceId());
		sendCommand("createIdentity", data);
	} else {
		QJsonObject data;
		data["userId"] = QString::fromUtf8(id);
		data["deviceId"] = QString::fromUtf8(loadDeviceId());
		sendCommand("identify", data);
	}
}

void WsRemoteConnector::disconnected()
{
	if(state == Operating)
		emit operationFailed("Connection closed");

	if(state != Closing) {
		auto delta = retry(false);
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
	emit remoteStateLoaded(false, {});
}

void WsRemoteConnector::binaryMessageReceived(const QByteArray &message)
{
	QJsonParseError error;
	auto doc = QJsonDocument::fromJson(message, &error);
	if(error.error != QJsonParseError::NoError) {
		qCWarning(LOG) << "Invalid data received. Parser error:"
					   << error.errorString();
		return;
	}

	auto obj = doc.object();
	auto data = obj["data"];
	if(obj["command"] == QStringLiteral("identified"))
		identified(data.toString());
	else if(obj["command"] == QStringLiteral("changeState"))
		changeState(data.toObject());
	else if(obj["command"] == QStringLiteral("notifyChanged"))
		notifyChanged(data.toObject());
	else if(obj["command"] == QStringLiteral("completed"))
		completed(data.toObject());
	else {
		qCWarning(LOG) << "Unkown command received:"
					   << obj["command"].toString();
	}
}

void WsRemoteConnector::error()
{
	qCWarning(LOG) << "Socket error"
				   << socket->errorString();
	socket->close();
}

void WsRemoteConnector::sslErrors(const QList<QSslError> &errors)
{
	foreach(auto error, errors) {
		qCWarning(LOG) << "SSL errors"
					   << error.errorString();
	}

	if(settings->value(keyVerifyPeer, true).toBool())
		socket->close();
}

void WsRemoteConnector::sendCommand(const QByteArray &command, const QJsonValue &data)
{
	QJsonObject message;
	message["command"] = QString::fromLatin1(command);
	message["data"] = data;

	QJsonDocument doc(message);
	socket->sendBinaryMessage(doc.toJson(QJsonDocument::Compact));
}

void WsRemoteConnector::identified(const QString &data)
{
	settings->setValue(keyUserIdentity, data.toUtf8());
	qCDebug(LOG) << "Identification successful";
	state = Idle;
	reload();
}

void WsRemoteConnector::changeState(const QJsonObject &data)
{
	if(data["success"].toBool()) {
		//reset retry
		retryIndex = 0;

		StateHolder::ChangeHash changeState;
		foreach(auto value, data["data"].toArray()) {
			auto obj = value.toObject();
			ObjectKey key;
			key.first = obj["type"].toString().toLatin1();
			key.second = obj["key"].toString();
			changeState.insert(key, obj["changed"].toBool() ? StateHolder::Changed : StateHolder::Deleted);
		}
		emit remoteStateLoaded(true, changeState);
	} else {
		auto delta = retry(true);
		qCWarning(LOG) << "Failed to load state with error:"
					   << data["error"].toString()
					   << ". Retrying in"
					   << delta / 1000
					   << "seconds";
	}
	state = Idle;
}

void WsRemoteConnector::notifyChanged(const QJsonObject &data)
{
	ObjectKey key;
	key.first = data["type"].toString().toLatin1();
	key.second = data["key"].toString();
	emit remoteDataChanged(key, data["changed"].toBool() ? StateHolder::Changed : StateHolder::Deleted);
}

void WsRemoteConnector::completed(const QJsonObject &result)
{
	if(result["success"].toBool())
		emit operationDone(result["data"]);
	else
		emit operationFailed(result["error"].toString());
	state = Idle;
}

int WsRemoteConnector::retry(bool reloadOnly)
{
	auto retryTimeout = 0;
	if(retryIndex >= timeouts.size())
		retryTimeout = timeouts.last();
	else
		retryTimeout = timeouts[retryIndex++];

	QTimer::singleShot(retryTimeout, this, [=](){
		if(reloadOnly)
			reload();
		else if(state == Disconnected)
			reconnect();
	});

	return retryTimeout;
}
