#ifndef QTDATASYNC_AUTHENTICATOR_P_H
#define QTDATASYNC_AUTHENTICATOR_P_H

#include "authenticator.h"
#include "googleoauthflow_p.h"

#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>

#include "auth_api.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT FirebaseAuthenticatorPrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(FirebaseAuthenticator)

public:
	static const QString FirebaseGroupKey;
	static const QString FirebaseRefreshTokenKey;
	static const QString FirebaseEmailKey;

	QPointer<Engine> engine;
	firebase::auth::ApiClient *api;
	QTimer *refreshTimer;

	QString localId;
	QString idToken;
	QString refreshToken;
	QDateTime expiresAt;
	QString email;

	static QString translateError(const QtDataSync::firebase::FirebaseError &error, int code);

	void _q_refreshToken();
	void _q_apiError(const QString &errorString, int errorCode, QtRestClient::RestReply::Error errorType);

	void startRefreshTimer();
	void loadFbConfig();
	void storeFbConfig();
	void clearFbConfig();
};

class Q_DATASYNC_EXPORT OAuthAuthenticatorPrivate : public FirebaseAuthenticatorPrivate
{
	Q_DECLARE_PUBLIC(OAuthAuthenticator)

public:
	static const QString OAuthGroupKey;
	static const QString OAuthRefreshTokenKey;

	bool preferNative = true;

	GoogleOAuthFlow *oAuthFlow = nullptr;
	bool aborted = false;

	void _q_firebaseSignIn();
	void _q_oAuthError(const QString &error, const QString &errorDescription);

	void loadOaConfig();
	void storeOaConfig();
	void clearOaConfig();
};

Q_DECLARE_LOGGING_CATEGORY(logFbAuth)
Q_DECLARE_LOGGING_CATEGORY(logOAuth)

}

#endif // QTDATASYNC_AUTHENTICATOR_P_H
