#ifndef QTDATASYNC_FIREBASEAUTHENTICATOR_H
#define QTDATASYNC_FIREBASEAUTHENTICATOR_H

#include "qtdatasync_global.h"
#include "authenticator.h"

#include <QtCore/QTimer>
#include <QtCore/QPointer>

#include "auth_api.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT FirebaseAuthenticator : public QObject
{
	Q_OBJECT

public:
	static const QString FirebaseGroupKey;
	static const QString FirebaseRefreshTokenKey;
	static const QString FirebaseEmailKey;

	static void logError(const QString &errorString, int errorCode, QtRestClient::RestReply::Error errorType);
	static QString translateError(const QtDataSync::firebase::auth::Error &error, int code);

	explicit FirebaseAuthenticator(IAuthenticator *authenticator,
								   const QString &apiKey,
								   QSettings *settings,
								   QNetworkAccessManager *nam,
								   const std::optional<QSslConfiguration> &sslConfig,
								   QObject *parent = nullptr);

	IAuthenticator *authenticator() const;

public Q_SLOTS:
	void signIn();
	void logOut();
	void deleteUser();

	void abortRequest();

Q_SIGNALS:
	void signInSuccessful(const QString &userId, const QString &idToken, QPrivateSignal = {});
	void signInFailed(QPrivateSignal = {});
	void accountDeleted(bool success, QPrivateSignal = {});

private Q_SLOTS:
	void refreshToken();
private:
	friend class IAuthenticator;

	IAuthenticator *_auth;
	QPointer<QSettings> _settings;
	firebase::auth::ApiClient *_api = nullptr;
	QTimer *_refreshTimer;

	QPointer<QNetworkReply> _lastReply;

	QString _localId;
	QString _idToken;
	QString _refreshToken;
	QDateTime _expiresAt;
	QString _email;

	void startRefreshTimer();
	void loadFbConfig();
	void storeFbConfig();
	void clearFbConfig();
};

Q_DECLARE_LOGGING_CATEGORY(logFbAuth)

}

#endif // QTDATASYNC_FIREBASEAUTHENTICATOR_H
