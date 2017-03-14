#ifndef WSAUTHENTICATOR_P_H
#define WSAUTHENTICATOR_P_H

#include "qdatasync_global.h"
#include "wsauthenticator.h"
#include "wsremoteconnector_p.h"

#include <QtCore/QSettings>

namespace QtDataSync {

class Q_DATASYNC_EXPORT WsAuthenticatorPrivate
{
public:
	QPointer<WsRemoteConnector> connector;
	QSettings *settings;

	bool connected;

	WsAuthenticatorPrivate(WsRemoteConnector *connector);
};

}

#endif // WSAUTHENTICATOR_P_H
