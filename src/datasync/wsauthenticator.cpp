#include "defaults.h"
#include "wsauthenticator.h"
#include "wsauthenticator_p.h"
#include "wsremoteconnector_p.h"

using namespace QtDataSync;

WsAuthenticator::WsAuthenticator(WsRemoteConnector *connector, Defaults *defaults, QObject *parent) :
	Authenticator(parent),
	d(new WsAuthenticatorPrivate(connector))
{
	d->settings = defaults->createSettings(this);

	connect(d->connector, &WsRemoteConnector::remoteStateLoaded,
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

bool WsAuthenticator::validateServerCertificate() const
{
	return d->settings->value(WsRemoteConnector::keyVerifyPeer).toBool();
}

QByteArray WsAuthenticator::userIdentity() const
{
	return d->settings->value(WsRemoteConnector::keyUserIdentity).toByteArray();
}

bool WsAuthenticator::isConnected() const
{
	return d->connected;
}

void WsAuthenticator::reconnect()
{
	QMetaObject::invokeMethod(d->connector, "reconnect");
}

void WsAuthenticator::setRemoteUrl(QUrl remoteUrl)
{
	d->settings->setValue(WsRemoteConnector::keyRemoteUrl, remoteUrl);
	d->settings->sync();
}

void WsAuthenticator::setCustomHeaders(QHash<QByteArray, QByteArray> customHeaders)
{
	d->settings->remove(WsRemoteConnector::keyHeadersGroup);
	d->settings->beginGroup(WsRemoteConnector::keyHeadersGroup);
	for(auto it = customHeaders.constBegin(); it != customHeaders.constEnd(); it++)
		d->settings->setValue(QString::fromUtf8(it.key()), it.value());
	d->settings->endGroup();
	d->settings->sync();
}

void WsAuthenticator::setValidateServerCertificate(bool validateServerCertificate)
{
	d->settings->setValue(WsRemoteConnector::keyVerifyPeer, validateServerCertificate);
	d->settings->sync();
}

GenericTask<void> WsAuthenticator::setUserIdentity(QByteArray userIdentity, bool clearLocalStore)
{
	return resetIdentity(userIdentity, clearLocalStore);
}

GenericTask<void> WsAuthenticator::resetUserIdentity(bool clearLocalStore)
{
	return resetIdentity(clearLocalStore);
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
