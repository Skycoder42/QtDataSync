#ifndef QTDATASYNC_AUTHENTICATOR_P_H
#define QTDATASYNC_AUTHENTICATOR_P_H

#include "authenticator.h"

#include <random>
#include <optional>

#include <QtCore/QRandomGenerator>
#include <QtCore/QLoggingCategory>

#include <QtNetworkAuth/QOAuth2AuthorizationCodeFlow>

#include "auth_api.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT GoogleOAuthFlow : public QOAuth2AuthorizationCodeFlow
{
public:
	GoogleOAuthFlow(quint16 port, QNetworkAccessManager *nam, QObject *parent);

	QString idToken() const;
	QUrl requestUrl() const;

private Q_SLOTS:
	void handleIdToken(const QVariantMap &values);

private:
	using BitEngine = std::independent_bits_engine<QRandomGenerator, std::numeric_limits<quint8>::digits, quint8>;

	QAbstractOAuthReplyHandler *_handler;
	QString _idToken;

	std::optional<BitEngine> _challengeEngine;
	QString _codeVerifier;

	QString createChallenge();
	void urlUnescape(QVariantMap *parameters, const QString &key) const;
};



class Q_DATASYNC_EXPORT FirebaseAuthenticatorPrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(FirebaseAuthenticator)

public:
	firebase::auth::ApiClient *api;

	QString localId;
	QString idToken;
	QString refreshToken;
	QDateTime expiresAt;
	QString email;

	static QString translateError(const QtDataSync::firebase::FirebaseError &error, int code);

	void _q_apiError(const QString &errorString, int errorCode, QtRestClient::RestReply::Error errorType);
};

class Q_DATASYNC_EXPORT OAuthAuthenticatorPrivate : public FirebaseAuthenticatorPrivate
{
	Q_DECLARE_PUBLIC(OAuthAuthenticator)

public:
	bool preferNative = true;

	GoogleOAuthFlow *oAuthFlow = nullptr;

	void _q_firebaseSignIn();
	void _q_oAuthError(const QString &error, const QString &errorDescription);
};

Q_DECLARE_LOGGING_CATEGORY(logFbAuth)
Q_DECLARE_LOGGING_CATEGORY(logOAuth)

}

#endif // QTDATASYNC_AUTHENTICATOR_P_H
