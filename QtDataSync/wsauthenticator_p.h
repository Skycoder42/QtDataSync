#ifndef WSAUTHENTICATOR_P_H
#define WSAUTHENTICATOR_P_H

#include "wsauthenticator.h"
#include "wsremoteconnector.h"

#include <QSettings>

namespace QtDataSync {

class WsAuthenticatorPrivate
{
public:
	QPointer<WsRemoteConnector> connector;
	QSettings *settings;

	bool connected;

	WsAuthenticatorPrivate(WsRemoteConnector *connector);
};

}

#endif // WSAUTHENTICATOR_P_H
