#ifndef WSAUTHENTICATOR_H
#define WSAUTHENTICATOR_H

#include "authenticator.h"
#include "qtdatasync_global.h"
#include <QObject>

namespace QtDataSync {

class WsRemoteConnector;

class WsAuthenticatorPrivate;
class WsAuthenticator : public Authenticator
{
	Q_OBJECT

	Q_PROPERTY(QByteArray userIdentity READ userIdentity WRITE setUserIdentity)

public:
	explicit WsAuthenticator(WsRemoteConnector *connector, QObject *parent = nullptr);
	~WsAuthenticator();

	QByteArray userIdentity() const;

public slots:
	void setUserIdentity(QByteArray userIdentity);

protected:
	RemoteConnector *connector() override;

private:
	QScopedPointer<WsAuthenticatorPrivate> d;
};

}

#endif // WSAUTHENTICATOR_H
