#ifndef QTDATASYNC_AUTHENTICATOR_H
#define QTDATASYNC_AUTHENTICATOR_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/engine.h"

#include <QtCore/qobject.h>

namespace QtRestClient {
class RestClient;
}

namespace QtDataSync {

class Q_DATASYNC_EXPORT IAuthenticator : public QObject
{
	Q_OBJECT

public:
	IAuthenticator(QObject *parent = nullptr);

public Q_SLOTS:
	virtual void signIn() = 0;
	virtual void logOut() = 0;
	virtual void deleteUser() = 0;

	virtual void abortSignIn() = 0;

Q_SIGNALS:
	void signInSuccessful(const QString &userId, const QString &idToken);
	void signInFailed(const QString &errorMessage);
	void accountDeleted(bool success);

protected:
	IAuthenticator(QObjectPrivate &dd, QObject *parent = nullptr);
};

class FirebaseAuthenticatorPrivate;
class Q_DATASYNC_EXPORT FirebaseAuthenticator : public IAuthenticator
{
	Q_OBJECT

public Q_SLOTS:
	void signIn() final;
	void logOut() override;
	void deleteUser() final;

protected:
	FirebaseAuthenticator(Engine *engine = nullptr);
	FirebaseAuthenticator(FirebaseAuthenticatorPrivate &dd, Engine *engine);

	virtual void firebaseSignIn() = 0;

	QtRestClient::RestClient *client() const;
	void completeSignIn(QString localId,
						QString idToken,
						QString refreshToken,
						const QDateTime &expiresAt,
						QString email);

private:
	Q_DECLARE_PRIVATE(FirebaseAuthenticator)

	Q_PRIVATE_SLOT(d_func(), void _q_refreshToken())
	Q_PRIVATE_SLOT(d_func(), void _q_apiError(const QString &, int, QtRestClient::RestReply::Error))
};

class OAuthAuthenticatorPrivate;
class Q_DATASYNC_EXPORT OAuthAuthenticator : public FirebaseAuthenticator
{
	Q_OBJECT

	Q_PROPERTY(bool preferNative READ doesPreferNative WRITE setPreferNative NOTIFY preferNativeChanged)

public:
	explicit OAuthAuthenticator(Engine *engine = nullptr);

	bool doesPreferNative() const;

	void logOut() override;

public Q_SLOTS:
	void abortSignIn() override;

	void setPreferNative(bool preferNative);

Q_SIGNALS:
	void signInRequested(const QUrl &authUrl);

	void preferNativeChanged(bool preferNative, QPrivateSignal);

protected:
	void firebaseSignIn() override;

private:
	Q_DECLARE_PRIVATE(OAuthAuthenticator)

	Q_PRIVATE_SLOT(d_func(), void _q_firebaseSignIn())
	Q_PRIVATE_SLOT(d_func(), void _q_oAuthError(const QString &, const QString &))
};

}

#endif // QTDATASYNC_AUTHENTICATOR_H
