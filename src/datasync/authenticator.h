#ifndef QTDATASYNC_AUTHENTICATOR_H
#define QTDATASYNC_AUTHENTICATOR_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/engine.h"

#include <QtCore/qobject.h>
#include <QtCore/qsettings.h>

namespace QtRestClient {
class RestClient;
}

namespace QtDataSync {

class FirebaseAuthenticatorPrivate;
class Q_DATASYNC_EXPORT FirebaseAuthenticator : public QObject
{
	Q_OBJECT

public Q_SLOTS:
	void signIn();
	virtual void logOut();
	virtual void deleteUser();

	void abortRequest();

Q_SIGNALS:
	void signInSuccessful(const QString &userId, const QString &idToken, QPrivateSignal = {});
	void signInFailed(const QString &errorMessage, QPrivateSignal = {});
	void accountDeleted(bool success, QPrivateSignal = {});

protected:
	FirebaseAuthenticator(const QString &apiKey, QSettings *settings, QObject *parent = nullptr);
	FirebaseAuthenticator(FirebaseAuthenticatorPrivate &dd, QObject *parent);

	virtual void firebaseSignIn() = 0;
	virtual void abortSignIn() = 0;

	QSettings *settings() const;
	QtRestClient::RestClient *client() const;

	void completeSignIn(QString localId,
						QString idToken,
						QString refreshToken,
						const QDateTime &expiresAt,
						QString email);
	void failSignIn(const QString &errorMessage);

private:
	Q_DECLARE_PRIVATE(FirebaseAuthenticator)

	Q_PRIVATE_SLOT(d_func(), void _q_refreshToken())
	Q_PRIVATE_SLOT(d_func(), void _q_apiError(const QString &, int, QtRestClient::RestReply::Error))
};

class OAuthAuthenticatorPrivate;
class Q_DATASYNC_EXPORT OAuthAuthenticator final : public FirebaseAuthenticator
{
	Q_OBJECT

	Q_PROPERTY(bool preferNative READ doesPreferNative WRITE setPreferNative NOTIFY preferNativeChanged)

public:
	explicit OAuthAuthenticator(const QString &firebaseApiKey,
								const QString &googleClientId,
								const QString &googleClientSecret,
								quint16 googleCallbackPort,
								QSettings *settings,
								QObject *parent = nullptr);

	bool doesPreferNative() const;

	void logOut() final;

public Q_SLOTS:
	void setPreferNative(bool preferNative);

Q_SIGNALS:
	void signInRequested(const QUrl &authUrl);

	void preferNativeChanged(bool preferNative, QPrivateSignal);

protected:
	void firebaseSignIn() final;
	void abortSignIn() final;

private:
	Q_DECLARE_PRIVATE(OAuthAuthenticator)

	Q_PRIVATE_SLOT(d_func(), void _q_firebaseSignIn())
	Q_PRIVATE_SLOT(d_func(), void _q_oAuthError(const QString &, const QString &))
};

}

#endif // QTDATASYNC_AUTHENTICATOR_H
