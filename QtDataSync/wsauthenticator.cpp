#include "defaults.h"
#include "wsauthenticator.h"
#include "wsauthenticator_p.h"
#include "wsremoteconnector.h"
using namespace QtDataSync;

WsAuthenticator::WsAuthenticator(WsRemoteConnector *connector, const QDir &storageDir, QObject *parent) :
	Authenticator(parent),
	d(new WsAuthenticatorPrivate(connector))
{
	d->settings = Defaults::settings(storageDir, this);

	connect(d->connector, &WsRemoteConnector::remoteStateChanged,
			this, &WsAuthenticator::updateConnected,
			Qt::QueuedConnection);
}

WsAuthenticator::~WsAuthenticator() {}

QUrl WsAuthenticator::remoteUrl() const
{
	return d->settings->value(WsRemoteConnector::keyRemoteUrl).toUrl();
}

QHash<QByteArray, QByteArray> WsAuthenticator::customHeaders() const
{
	d->settings->beginGroup(WsRemoteConnector::keyHeadersGroup);
	auto keys = d->settings->childKeys();
	QHash<QByteArray, QByteArray> headers;
	foreach(auto key, keys)
		headers.insert(key.toUtf8(), d->settings->value(key).toByteArray());
	d->settings->endGroup();
	return headers;
}

QByteArray WsAuthenticator::userIdentity() const
{
	return d->settings->value(WsRemoteConnector::keyUserIdentity).toByteArray();
}

bool WsAuthenticator::isConnected() const
{
	return d->connected;
}

void WsAuthenticator::setRemoteUrl(QUrl remoteUrl)
{
	d->settings->setValue(WsRemoteConnector::keyRemoteUrl, remoteUrl);
	d->settings->sync();
	QMetaObject::invokeMethod(d->connector, "reconnect");
}

void WsAuthenticator::setCustomHeaders(QHash<QByteArray, QByteArray> customHeaders)
{
	d->settings->remove(WsRemoteConnector::keyHeadersGroup);
	d->settings->beginGroup(WsRemoteConnector::keyHeadersGroup);
	for(auto it = customHeaders.constBegin(); it != customHeaders.constEnd(); it++)
		d->settings->setValue(QString::fromUtf8(it.key()), it.value());
	d->settings->endGroup();
	d->settings->sync();
	QMetaObject::invokeMethod(d->connector, "reconnect");
}

void WsAuthenticator::setUserIdentity(QByteArray userIdentity)
{
	d->settings->setValue(WsRemoteConnector::keyUserIdentity, userIdentity);
	d->settings->sync();
	QMetaObject::invokeMethod(d->connector, "reconnect");
}

RemoteConnector *WsAuthenticator::connector()
{
	return d->connector;
}

void WsAuthenticator::updateConnected(bool connected)
{
	d->connected = connected;
	emit connectedChanged(connected);
}

WsAuthenticatorPrivate::WsAuthenticatorPrivate(WsRemoteConnector *connector) :
	connector(connector),
	settings(nullptr),
	connected(false)
{}
