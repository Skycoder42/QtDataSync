#ifndef QTDATASYNC_WSAUTHENTICATOR_H
#define QTDATASYNC_WSAUTHENTICATOR_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/authenticator.h"
#include "QtDataSync/defaults.h"

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qdir.h>

namespace QtDataSync {

class WsRemoteConnector;

class WsAuthenticatorPrivate;
//! The authenticator for the default RemoteConnector implementation
class Q_DATASYNC_EXPORT WsAuthenticator : public Authenticator
{
	Q_OBJECT

	//! The websocket url of the remote server
	Q_PROPERTY(QUrl remoteUrl READ remoteUrl WRITE setRemoteUrl)
	//! A hash with custom additional http headers for the initial request
	Q_PROPERTY(QHash<QByteArray, QByteArray> customHeaders READ customHeaders WRITE setCustomHeaders)
	//! Specify, whether server certificates should be validated
	Q_PROPERTY(bool validateServerCertificate READ validateServerCertificate WRITE setValidateServerCertificate)
	//! Holds the current user identity, i.e. the account
	Q_PROPERTY(QByteArray userIdentity READ userIdentity WRITE setUserIdentity RESET resetUserIdentity)
	//! Holds the secret that is required to use the server
	Q_PROPERTY(QString serverSecret READ serverSecret WRITE setServerSecret)
	//! Specifies, whether the remote is currently connected or not
	Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
	//! Constructor, used internally
	explicit WsAuthenticator(WsRemoteConnector *connector,
							 QtDataSync::Defaults *defaults,
							 QObject *parent = nullptr);
	//! Destructor
	~WsAuthenticator();

	void exportUserDataImpl(QIODevice *device) const override;
	GenericTask<void> importUserDataImpl(QIODevice *device) override;

	//! @readAcFn{WsAuthenticator::remoteUrl}
	QUrl remoteUrl() const;
	//! @readAcFn{WsAuthenticator::customHeaders}
	QHash<QByteArray, QByteArray> customHeaders() const;
	//! @readAcFn{WsAuthenticator::validateServerCertificate}
	bool validateServerCertificate() const;
	//! @readAcFn{WsAuthenticator::userIdentity}
	QByteArray userIdentity() const;
	//! @readAcFn{WsAuthenticator::serverSecret}
	QString serverSecret() const;
	//! @readAcFn{WsAuthenticator::connected}
	bool isConnected() const;

public Q_SLOTS:
	//! Tells the connector to reconnect to the server with updated data
	void reconnect();

	//! @writeAcFn{WsAuthenticator::remoteUrl}
	void setRemoteUrl(QUrl remoteUrl);
	//! @writeAcFn{WsAuthenticator::customHeaders}
	void setCustomHeaders(QHash<QByteArray, QByteArray> customHeaders);
	//! @writeAcFn{WsAuthenticator::validateServerCertificate}
	void setValidateServerCertificate(bool validateServerCertificate);
	//! @writeAcFn{WsAuthenticator::userIdentity}
	GenericTask<void> setUserIdentity(QByteArray userIdentity, bool clearLocalStore = true);
	//! @resetAcFn{WsAuthenticator::userIdentity}
	GenericTask<void> resetUserIdentity(bool clearLocalStore = true);
	//! @writeAcFn{WsAuthenticator::serverSecret}
	void setServerSecret(QString serverSecret);

Q_SIGNALS:
	//! @notifyAcFn{WsAuthenticator::connected}
	void connectedChanged(bool connected);

protected:
	RemoteConnector *connector() override;

private Q_SLOTS:
	void updateConnected(bool connected);

private:
	QScopedPointer<WsAuthenticatorPrivate> d;
};

}

#endif // QTDATASYNC_WSAUTHENTICATOR_H
