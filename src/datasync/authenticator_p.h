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

class Q_DATASYNC_EXPORT IdTokenFlow : public QOAuth2AuthorizationCodeFlow
{
public:
	IdTokenFlow(quint16 port, QObject *parent);

	QString idToken() const;

private Q_SLOTS:
	void handleIdToken(const QVariantMap &values);

private:
	QString _idToken;
};

class Q_DATASYNC_EXPORT OAuthAuthenticatorPrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(OAuthAuthenticator)

public:
	using BitEngine = std::independent_bits_engine<QRandomGenerator, std::numeric_limits<quint8>::digits, quint8>;

	firebase::auth::ApiClient *api;

	QString localId;
	QString idToken;
	QString refreshToken;
	QDateTime expireDate;

	bool preferNative = true;

	IdTokenFlow *oAuthFlow = nullptr;
	std::optional<BitEngine> challengeEngine;
	QString codeVerifier;

	static QString translateError(const QtDataSync::firebase::FirebaseError &error, int code);

	void initOAuth();
	void runOAuth();

	QString createChallenge();
	void urlUnescape(QVariantMap *parameters, const QString &key) const;

	void firebaseAuth();
};

Q_DECLARE_LOGGING_CATEGORY(logAuth)

}

#endif // QTDATASYNC_AUTHENTICATOR_P_H
