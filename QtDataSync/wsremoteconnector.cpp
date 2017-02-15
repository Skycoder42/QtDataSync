#include "defaults.h"
#include "wsauthenticator.h"
#include "wsremoteconnector.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
using namespace QtDataSync;

#define LOG Defaults::loggingCategory(storageDir)

const QString WsRemoteConnector::keyRemoteUrl(QStringLiteral("RemoteConnector/remoteUrl"));
const QString WsRemoteConnector::keyHeadersGroup(QStringLiteral("RemoteConnector/headers"));
const QString WsRemoteConnector::keyVerifyPeer(QStringLiteral("RemoteConnector/verifyPeer"));
const QString WsRemoteConnector::keyUserIdentity(QStringLiteral("RemoteConnector/userIdentity"));

WsRemoteConnector::WsRemoteConnector(QObject *parent) :
	RemoteConnector(parent),
	socket(nullptr),
	settings(nullptr),
	state(Disconnected)
{}

void WsRemoteConnector::initialize(const QDir &storageDir)
{
	this->storageDir = storageDir;
	RemoteConnector::initialize(storageDir);
	settings = Defaults::settings(storageDir, this);
	reconnect();
}

void WsRemoteConnector::finalize(const QDir &storageDir)
{
	RemoteConnector::finalize(storageDir);
}

Authenticator *WsRemoteConnector::createAuthenticator(const QDir &storageDir, QObject *parent)
{
	return new WsAuthenticator(this, storageDir, parent);
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

void WsRemoteConnector::download(const ObjectKey &key, const QByteArray &keyProperty)
{
	qCDebug(LOG) << Q_FUNC_INFO << key;
	emit operationDone(QJsonObject());
}

void WsRemoteConnector::upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty)
{
	qCDebug(LOG) << Q_FUNC_INFO << key;
	emit operationDone();
}

void WsRemoteConnector::remove(const ObjectKey &key, const QByteArray &keyProperty)
{
	qCDebug(LOG) << Q_FUNC_INFO << key;
	emit operationDone();
}

void WsRemoteConnector::markUnchanged(const ObjectKey &key, const QByteArray &keyProperty)
{
	qCDebug(LOG) << Q_FUNC_INFO << key;
	emit operationDone();

}

void WsRemoteConnector::resetDeviceId()
{
	RemoteConnector::resetDeviceId();
	reconnect();
}

void WsRemoteConnector::connected()
{
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
	state = Disconnected;
	socket->deleteLater();
	socket = nullptr;
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
	if(obj["command"] == QStringLiteral("identity")) {
		auto identity = obj["data"].toString().toUtf8();
		settings->setValue(keyUserIdentity, identity);
	}
}

void WsRemoteConnector::error()
{
	qCWarning(LOG) << "Socket error"
				   << socket->errorString();
	state = Closing;
	socket->close();
}

void WsRemoteConnector::sslErrors(const QList<QSslError> &errors)
{
	foreach(auto error, errors) {
		qCWarning(LOG) << "SSL errors"
					   << error.errorString();
	}
	if(settings->value(keyVerifyPeer, true).toBool()) {
		state = Closing;
		socket->close();
	}
}

void WsRemoteConnector::sendCommand(const QByteArray &command, const QJsonValue &data)
{
	QJsonObject message;
	message["command"] = QString::fromLatin1(command);
	message["data"] = data;

	QJsonDocument doc(message);
	socket->sendBinaryMessage(doc.toJson(QJsonDocument::Compact));
}
