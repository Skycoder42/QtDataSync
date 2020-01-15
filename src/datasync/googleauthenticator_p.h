#ifndef QTDATASYNC_GOOGLEAUTHENTICATOR_P_H
#define QTDATASYNC_GOOGLEAUTHENTICATOR_P_H

#include "googleauthenticator.h"
#include "authenticator_p.h"
#include "googleoauthflow_p.h"

#include <QtCore/QLoggingCategory>

#include "auth_api.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT GoogleAuthenticatorPrivate : public IAuthenticatorPrivate
{
	Q_DECLARE_PUBLIC(GoogleAuthenticator)

public:
	static const QString OAuthGroupKey;
	static const QString OAuthRefreshTokenKey;

	GoogleOAuthFlow *oAuthFlow = nullptr;
	firebase::auth::ApiClient *api = nullptr;

	bool aborted = false;
	QPointer<QNetworkReply> lastReply;

	bool preferNative = true;

	void _q_firebaseSignIn();
	void _q_oAuthError(const QString &error, const QString &errorDescription);

	void loadOaConfig();
	void storeOaConfig();
	void clearOaConfig();
};

Q_DECLARE_LOGGING_CATEGORY(logOAuth)

}

#endif // QTDATASYNC_GOOGLEAUTHENTICATOR_P_H
