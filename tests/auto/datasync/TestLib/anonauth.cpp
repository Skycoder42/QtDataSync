#include "anonauth.h"
#include <QtRestClient/RestClient>
#include <QtRestClient/RestClass>
#include <QtRestClient/RestReply>
using namespace QtDataSync;
using namespace QtRestClient;

AnonAuth::AnonAuth(const QString &apiKey, QSettings *settings, QObject *parent) :
	FirebaseAuthenticator{apiKey, settings, parent}
{}

void AnonAuth::abortSignIn()
{
	if (_authReply)
		_authReply->abort();
}

void AnonAuth::firebaseSignIn()
{
	qDebug() << "Creating anonymous user";
	auto reply = client()->rootClass()->callRaw(RestClass::PostVerb,
												QStringLiteral("accounts:signUp"),
												QJsonObject {
													{QStringLiteral("returnSecureToken"), true}
												});
	_authReply = reply->networkReply();
	reply->onSucceeded(this, [this](const QJsonObject &data) {
		qDebug() << "Anonymous user created";
		completeSignIn(data.value(QStringLiteral("localId")).toString(),
					   data.value(QStringLiteral("idToken")).toString(),
					   data.value(QStringLiteral("refreshToken")).toString(),
					   QDateTime::currentDateTimeUtc().addSecs(data.value(QStringLiteral("expiresIn")).toString().toInt()),
					   data.value(QStringLiteral("email")).toString());
	})->onAllErrors(this, [this](const QString &error, int, QtRestClient::RestReply::Error){
		emit signInFailed(error);
	});
}
