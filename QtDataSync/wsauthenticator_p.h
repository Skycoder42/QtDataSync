#ifndef WSAUTHENTICATOR_P_H
#define WSAUTHENTICATOR_P_H

#include "wsauthenticator.h"
#include "wsremoteconnector.h"

namespace QtDataSync {

class WsAuthenticatorPrivate
{
public:
	QPointer<WsRemoteConnector> connector;
	QByteArray userIdentity;

	WsAuthenticatorPrivate(WsRemoteConnector *connector);
};

}

#endif // WSAUTHENTICATOR_P_H
