#ifndef QTDATASYNC_WSAUTHENTICATOR_P_H
#define QTDATASYNC_WSAUTHENTICATOR_P_H

#include "qtdatasync_global.h"
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

#endif // QTDATASYNC_WSAUTHENTICATOR_P_H
