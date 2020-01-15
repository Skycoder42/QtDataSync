#include "anonauth.h"
#include <QtRestClient/RestClient>
#include <QtRestClient/RestClass>
#include <QtRestClient/RestReply>
using namespace QtDataSync;
using namespace QtRestClient;

AnonAuth::AnonAuth(QObject *parent) :
	IAuthenticator{parent}
{}

void AnonAuth::signIn()
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
			failSignIn(error);
		});
}

void AnonAuth::logOut() {}

void AnonAuth::abortRequest()
{
	if (_authReply)
		_authReply->abort();
}
