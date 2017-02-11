#ifndef WSAUTHENTICATOR_H
#define WSAUTHENTICATOR_H

#include "authenticator.h"
#include "qtdatasync_global.h"
#include <QObject>
#include <QUrl>
#include <qdir.h>

namespace QtDataSync {

class WsRemoteConnector;

class WsAuthenticatorPrivate;
class WsAuthenticator : public Authenticator
{
	Q_OBJECT

	Q_PROPERTY(QUrl remoteUrl READ remoteUrl WRITE setRemoteUrl)
	Q_PROPERTY(QHash<QByteArray, QByteArray> customHeaders READ customHeaders WRITE setCustomHeaders)
	Q_PROPERTY(bool validateServerCertificate READ validateServerCertificate WRITE setValidateServerCertificate)
	Q_PROPERTY(QByteArray userIdentity READ userIdentity WRITE setUserIdentity)
	Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
	explicit WsAuthenticator(WsRemoteConnector *connector,
							 const QDir &storageDir,
							 QObject *parent = nullptr);
	~WsAuthenticator();

	QUrl remoteUrl() const;
	QHash<QByteArray, QByteArray> customHeaders() const;
	bool validateServerCertificate() const;
	QByteArray userIdentity() const;
	bool isConnected() const;

public slots:
	void reconnect();

	void setRemoteUrl(QUrl remoteUrl);
	void setCustomHeaders(QHash<QByteArray, QByteArray> customHeaders);
	void setValidateServerCertificate(bool validateServerCertificate);
	void setUserIdentity(QByteArray userIdentity);

signals:
	void connectedChanged(bool connected);

protected:
	RemoteConnector *connector() override;

private slots:
	void updateConnected(bool connected);

private:
	QScopedPointer<WsAuthenticatorPrivate> d;
};

}

#endif // WSAUTHENTICATOR_H
