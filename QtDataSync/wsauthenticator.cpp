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

QByteArray WsAuthenticator::userIdentity() const
{
	return d->settings->value(WsRemoteConnector::keyUserIdentity).toByteArray();
}

bool WsAuthenticator::isConnected() const
{
	return d->connected;
}

void WsAuthenticator::setUserIdentity(QByteArray userIdentity)
{
	d->settings->setValue(WsRemoteConnector::keyUserIdentity, userIdentity);
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
