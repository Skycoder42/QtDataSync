#include "anonauth.h"
#include <QtRestClient/RestClient>
#include <QtRestClient/RestClass>
#include <QtRestClient/RestReply>
using namespace QtDataSync;
using namespace QtRestClient;
using namespace std::chrono;

AnonAuth::AnonAuth(QObject *parent) :
	IAuthenticator{parent}
{}

void AnonAuth::setOverwriteExpire(std::optional<std::chrono::seconds> delta)
{
	_expireOw = std::move(delta);
}

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
			 const auto delta = _expireOw.value_or(seconds{data.value(QStringLiteral("expiresIn")).toString().toInt()});
			 qDebug() << "Anonymous user created (expires in:" << delta.count() << "seconds)";
			 completeSignIn(data.value(QStringLiteral("localId")).toString(),
							data.value(QStringLiteral("idToken")).toString(),
							data.value(QStringLiteral("refreshToken")).toString(),
							QDateTime::currentDateTimeUtc().addSecs(delta.count()),
							data.value(QStringLiteral("email")).toString());
		 })->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error type){
			qCritical() << "AnonAuth:" << type << code << error;
			failSignIn();
		});
}

void AnonAuth::logOut() {}

void AnonAuth::abortRequest()
{
	if (_authReply)
		_authReply->abort();
}
