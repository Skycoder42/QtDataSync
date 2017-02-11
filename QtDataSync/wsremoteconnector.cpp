#include "defaults.h"
#include "wsauthenticator.h"
#include "wsremoteconnector.h"

#include <QDebug>
#include <QJsonObject>
using namespace QtDataSync;

const QString WsRemoteConnector::keyRemoteUrl(QStringLiteral("RemoteConnector/remoteUrl"));
const QString WsRemoteConnector::keyHeadersGroup(QStringLiteral("RemoteConnector/headers"));
const QString WsRemoteConnector::keyVerifyPeer(QStringLiteral("RemoteConnector/verifyPeer"));
const QString WsRemoteConnector::keyUserIdentity(QStringLiteral("RemoteConnector/userIdentity"));

WsRemoteConnector::WsRemoteConnector(QObject *parent) :
	RemoteConnector(parent),
	socket(nullptr),
	settings(nullptr),
	connecting(false)
{}

void WsRemoteConnector::initialize(const QDir &storageDir)
{
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
	if(connecting)
		return;

	connecting = true;
	if(socket) {
		connect(socket, &QWebSocket::destroyed,
				this, &WsRemoteConnector::reconnect,
				Qt::QueuedConnection);
		socket->close();
	} else {
		settings->sync();

		auto remoteUrl = settings->value(keyRemoteUrl).toUrl();
		if(!remoteUrl.isValid()) {
			connecting = false;
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

		connect(socket, &QWebSocket::disconnected, this, [this](){
			socket->deleteLater();
			socket = nullptr;
		});

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
	qDebug() << Q_FUNC_INFO << key;
	emit operationDone(QJsonObject());
}

void WsRemoteConnector::upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty)
{
	qDebug() << Q_FUNC_INFO << key;
	emit operationDone();
}

void WsRemoteConnector::remove(const ObjectKey &key, const QByteArray &keyProperty)
{
	qDebug() << Q_FUNC_INFO << key;
	emit operationDone();
}

void WsRemoteConnector::markUnchanged(const ObjectKey &key, const QByteArray &keyProperty)
{
	qDebug() << Q_FUNC_INFO << key;
	emit operationDone();

}

void WsRemoteConnector::resetDeviceId()
{
	RemoteConnector::resetDeviceId();
	reconnect();
}
