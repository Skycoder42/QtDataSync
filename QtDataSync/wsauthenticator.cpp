#include "wsauthenticator.h"
#include "wsauthenticator_p.h"
#include "wsremoteconnector.h"
using namespace QtDataSync;

WsAuthenticator::WsAuthenticator(WsRemoteConnector *connector, QObject *parent) :
	Authenticator(parent),
	d(new WsAuthenticatorPrivate(connector))
{}

WsAuthenticator::~WsAuthenticator()
{

}

QByteArray WsAuthenticator::userIdentity() const
{
	return d->userIdentity;
}

void WsAuthenticator::setUserIdentity(QByteArray userIdentity)
{
	d->userIdentity = userIdentity;
}

RemoteConnector *WsAuthenticator::connector()
{
	return d->connector;
}
