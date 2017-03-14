#ifndef WSAUTHENTICATOR_H
#define WSAUTHENTICATOR_H

#include "QtDataSync/qdatasync_global.h"
#include "QtDataSync/authenticator.h"
#include "QtDataSync/defaults.h"

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qdir.h>

namespace QtDataSync {

class WsRemoteConnector;

class WsAuthenticatorPrivate;
class Q_DATASYNC_EXPORT WsAuthenticator : public Authenticator
{
	Q_OBJECT

	Q_PROPERTY(QUrl remoteUrl READ remoteUrl WRITE setRemoteUrl)
	Q_PROPERTY(QHash<QByteArray, QByteArray> customHeaders READ customHeaders WRITE setCustomHeaders)
	Q_PROPERTY(bool validateServerCertificate READ validateServerCertificate WRITE setValidateServerCertificate)
	Q_PROPERTY(QByteArray userIdentity READ userIdentity WRITE setUserIdentity RESET resetUserIdentity)
	Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
	explicit WsAuthenticator(WsRemoteConnector *connector,
							 QtDataSync::Defaults *defaults,
							 QObject *parent = nullptr);
	~WsAuthenticator();

	QUrl remoteUrl() const;
	QHash<QByteArray, QByteArray> customHeaders() const;
	bool validateServerCertificate() const;
	QByteArray userIdentity() const;
	bool isConnected() const;

public Q_SLOTS:
	void reconnect();

	void setRemoteUrl(QUrl remoteUrl);
	void setCustomHeaders(QHash<QByteArray, QByteArray> customHeaders);
	void setValidateServerCertificate(bool validateServerCertificate);
	GenericTask<void> setUserIdentity(QByteArray userIdentity, bool clearLocalStore = true);
	GenericTask<void> resetUserIdentity(bool clearLocalStore = true);

Q_SIGNALS:
	void connectedChanged(bool connected);

protected:
	RemoteConnector *connector() override;

private Q_SLOTS:
	void updateConnected(bool connected);

private:
	QScopedPointer<WsAuthenticatorPrivate> d;
};

}

#endif // WSAUTHENTICATOR_H
