#include "authenticator.h"
#include "authenticator_p.h"
#include "engine_p.h"
#include <QtCore/QAtomicInteger>

using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::auth;
using namespace std::chrono;
using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(QtDataSync::logFbAuth, "qt.datasync.FirebaseAuthenticator")
Q_LOGGING_CATEGORY(QtDataSync::logOAuth, "qt.datasync.OAuthAuthenticator")

void FirebaseAuthenticator::signIn()
{
	Q_D(FirebaseAuthenticator);
	if (!d->idToken.isEmpty() && d->expiresAt > QDateTime::currentDateTimeUtc()) { // token valid and not expired -> done
		qCDebug(logFbAuth) << "Firebase-token still valid - sign in successfull";
		Q_EMIT signInSuccessful(d->localId, d->idToken);
	} else if (!d->refreshToken.isEmpty()) // has refresh token -> refresh
		d->_q_refreshToken();
	else { // no token and no refresh -> normal sign in
		qCDebug(logFbAuth) << "Firebase-token invalid or expired - signing in...";
		d->clearFbConfig();
		firebaseSignIn();
	}
}

void FirebaseAuthenticator::logOut()
{
	Q_D(FirebaseAuthenticator);
	d->refreshTimer->stop();
	d->localId.clear();
	d->idToken.clear();
	d->refreshToken.clear();
	d->email.clear();
	d->clearFbConfig();
}

void FirebaseAuthenticator::deleteUser()
{
	Q_D(FirebaseAuthenticator);
	if (d->idToken.isEmpty()) {
		qCWarning(logFbAuth) << "Cannot delete account without beeing logged in";
		Q_EMIT accountDeleted(false);
		return;
	}

	const auto reply = d->api->deleteAccount(d->idToken);
	d->lastReply = reply->networkReply();
	reply->onSucceeded(this, [this](int) {
		qCDebug(logFbAuth) << "Firebase account delete successful";
		logOut();
		Q_EMIT accountDeleted(true);
	});
	reply->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType){
		Q_D(FirebaseAuthenticator);
		d->_q_apiError(error, code, errorType);
		qCWarning(logFbAuth) << "Firebase account delete failed";
		Q_EMIT accountDeleted(false);
	}, &FirebaseAuthenticatorPrivate::translateError);
}

void FirebaseAuthenticator::abortRequest()
{
	Q_D(FirebaseAuthenticator);
	if (d->lastReply)
		d->lastReply->abort();
	else
		abortSignIn();
}

FirebaseAuthenticator::FirebaseAuthenticator(const QString &apiKey, QSettings *settings, QObject *parent) :
	FirebaseAuthenticator{*new FirebaseAuthenticatorPrivate{}, parent}
{
	Q_D(FirebaseAuthenticator);
	d->settings = settings;
	d->initFbApi(apiKey);
}

FirebaseAuthenticator::FirebaseAuthenticator(FirebaseAuthenticatorPrivate &dd, QObject *parent) :
	QObject{dd, parent}
{
#ifdef Q_ATOMIC_INT8_IS_SUPPORTED
	static QAtomicInteger<bool> authReg = false;
#else
	static QAtomicInteger<quint16> authReg = false;
#endif
	if (authReg.testAndSetOrdered(false, true)) {
		qRegisterMetaType<ErrorContent>("ErrorContent");
		QtJsonSerializer::SerializerBase::registerListConverters<ErrorContent>();
	}

	Q_D(FirebaseAuthenticator);
	d->refreshTimer = new QTimer{this};
	d->refreshTimer->setSingleShot(true);
	d->refreshTimer->setTimerType(Qt::VeryCoarseTimer);
	QObjectPrivate::connect(d->refreshTimer, &QTimer::timeout,
							d, &FirebaseAuthenticatorPrivate::_q_refreshToken);
}

QSettings *FirebaseAuthenticator::settings() const
{
	Q_D(const FirebaseAuthenticator);
	return d->settings;
}

QtRestClient::RestClient *FirebaseAuthenticator::client() const
{
	Q_D(const FirebaseAuthenticator);
	return d->api->restClient();
}

void FirebaseAuthenticator::completeSignIn(QString localId, QString idToken, QString refreshToken, const QDateTime &expiresAt, QString email)
{
	Q_D(FirebaseAuthenticator);
	d->localId = std::move(localId);
	d->idToken = std::move(idToken);
	d->refreshToken = std::move(refreshToken);
	d->expiresAt = expiresAt.toUTC();
	d->email = std::move(email);
	d->storeFbConfig();
	d->startRefreshTimer();
	Q_EMIT signInSuccessful(d->localId, d->idToken);
}

void FirebaseAuthenticator::failSignIn(const QString &errorMessage)
{
	Q_EMIT signInFailed(errorMessage);
}



const QString FirebaseAuthenticatorPrivate::FirebaseGroupKey = QStringLiteral("auth/firebase");
const QString FirebaseAuthenticatorPrivate::FirebaseRefreshTokenKey = QStringLiteral("auth/firebase/refreshToken");
const QString FirebaseAuthenticatorPrivate::FirebaseEmailKey = QStringLiteral("auth/firebase/email");

QString FirebaseAuthenticatorPrivate::translateError(const Error &error, int code)
{
	if (const auto msg = error.error().message(); !msg.isEmpty())
		return msg;
	for (const auto &subMsg : error.error().errors()) {
		if (const auto msg = subMsg.message(); !msg.isEmpty())
			return msg;
	}
	return QStringLiteral("Request failed with status code %1").arg(code);
}

void FirebaseAuthenticatorPrivate::initFbApi(const QString &apiKey)
{
	Q_Q(FirebaseAuthenticator);
	auto client = new QtRestClient::RestClient{QtRestClient::RestClient::DataMode::Json, q};
	client->setModernAttributes();
	client->addRequestAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	client->setBaseUrl(QUrl{QStringLiteral("https://identitytoolkit.googleapis.com")});
	client->setApiVersion(QVersionNumber{1});
	client->addGlobalParameter(QStringLiteral("key"), apiKey);
	api = new ApiClient{client->rootClass(), q};
	loadFbConfig();
}

void FirebaseAuthenticatorPrivate::_q_refreshToken()
{
	Q_Q(FirebaseAuthenticator);
	qCDebug(logFbAuth) << "Firebase-token expired - refreshing...";
	const auto reply = api->refreshToken(refreshToken);
	lastReply = reply->networkReply();
	reply->onSucceeded(q, [this](int, const RefreshResponse &response) {
		Q_Q(FirebaseAuthenticator);
		if (response.token_type() != QStringLiteral("Bearer")) {
			qCCritical(logFbAuth) << "Invalid token type" << response.token_type();
			Q_EMIT q->signInFailed(FirebaseAuthenticator::tr("Failed renew firebase connection! Please try to sign in again."));
			return;
		}
		qCDebug(logFbAuth) << "Firebase-token refresh successful";
		localId = response.user_id();
		idToken = response.id_token();
		refreshToken = response.refresh_token();
		expiresAt = QDateTime::currentDateTimeUtc().addSecs(response.expires_in());
		storeFbConfig();
		startRefreshTimer();
		Q_EMIT q->signInSuccessful(localId, idToken);
	});
	reply->onAllErrors(q, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType){
		Q_Q(FirebaseAuthenticator);
		_q_apiError(error, code, errorType);
		qCWarning(logFbAuth) << "Refreshing the firebase-token failed - signing in again...";
		clearFbConfig();
		q->firebaseSignIn();
	}, &FirebaseAuthenticatorPrivate::translateError);
}

void FirebaseAuthenticatorPrivate::_q_apiError(const QString &errorString, int errorCode, QtRestClient::RestReply::Error errorType)
{
	qCWarning(logFbAuth) << "API request failed with reason" << errorType
						 << "and code" << errorCode
						 << "- error message:" << qUtf8Printable(errorString);
}

void FirebaseAuthenticatorPrivate::startRefreshTimer()
{
	// schedule a refresh, one minute early
	auto expireDelta = milliseconds{QDateTime::currentDateTimeUtc().msecsTo(expiresAt)} - 1min;
	if (expireDelta < 0ms)
		_q_refreshToken();
	else {
		refreshTimer->start(expireDelta);
		qCDebug(logFbAuth) << "Scheduled token refresh in"
						   << expireDelta.count()
						   << "ms";
	}
}

void FirebaseAuthenticatorPrivate::loadFbConfig()
{
	refreshToken = settings->value(FirebaseRefreshTokenKey).toString();
	email = settings->value(FirebaseEmailKey).toString();
}

void FirebaseAuthenticatorPrivate::storeFbConfig()
{
	settings->setValue(FirebaseRefreshTokenKey, refreshToken);
	settings->setValue(FirebaseEmailKey, email);
}

void FirebaseAuthenticatorPrivate::clearFbConfig()
{
	settings->remove(FirebaseGroupKey);
}




OAuthAuthenticator::OAuthAuthenticator(const QString &firebaseApiKey, const QString &googleClientId, const QString &googleClientSecret, quint16 googleCallbackPort, QSettings *settings, QObject *parent) :
	FirebaseAuthenticator{*new OAuthAuthenticatorPrivate{}, parent}
{
	Q_D(OAuthAuthenticator);
	d->settings = settings;
	d->initFbApi(firebaseApiKey);
	d->initOAuth(googleClientId, googleClientSecret, googleCallbackPort);
}

bool OAuthAuthenticator::doesPreferNative() const
{
	Q_D(const OAuthAuthenticator);
	return d->preferNative;
}

void OAuthAuthenticator::logOut()
{
	FirebaseAuthenticator::logOut();
	Q_D(OAuthAuthenticator);
	d->oAuthFlow->setIdToken({});
	d->oAuthFlow->setRefreshToken({});
	d->clearOaConfig();
}

void OAuthAuthenticator::setPreferNative(bool preferNative)
{
	Q_D(OAuthAuthenticator);
	if (d->preferNative == preferNative)
		return;

	d->preferNative = preferNative;
	emit preferNativeChanged(d->preferNative, {});
}

void OAuthAuthenticator::firebaseSignIn()
{
	Q_D(OAuthAuthenticator);
	d->aborted = false;
	if (!d->oAuthFlow->idToken().isEmpty() && d->oAuthFlow->expirationAt().toUTC() > QDateTime::currentDateTimeUtc()) { // valid id token -> done
		qCDebug(logOAuth) << "OAuth-token still valid - sign in successfull";
		d->_q_firebaseSignIn();
	} else if (!d->oAuthFlow->refreshToken().isEmpty()) { // valid refresh token -> do the refresh
		qCDebug(logOAuth) << "OAuth-token expired - refreshing...";
		d->oAuthFlow->refreshAccessToken();
	} else { // no token or refresh token -> run full oauth flow
		qCDebug(logOAuth) << "OAuth-token invalid or expired - signing in...";
		d->clearOaConfig();
		d->oAuthFlow->grant();
	}
}

void OAuthAuthenticator::abortSignIn()
{
	Q_D(OAuthAuthenticator);
	d->aborted = true;
	failSignIn(tr("Sign-In aborted!"));
}



const QString OAuthAuthenticatorPrivate::OAuthGroupKey = QStringLiteral("auth/google-oauth");
const QString OAuthAuthenticatorPrivate::OAuthRefreshTokenKey = QStringLiteral("auth/google-oauth/refreshToken");

void OAuthAuthenticatorPrivate::initOAuth(const QString &clientId, const QString &clientSecret, quint16 callbackPort)
{
	Q_Q(OAuthAuthenticator);
	oAuthFlow = new GoogleOAuthFlow{callbackPort, api->restClient()->manager(), q};
	oAuthFlow->setScope(QStringLiteral("openid email https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/firebase.database"));
	oAuthFlow->setClientIdentifier(clientId);
	oAuthFlow->setClientIdentifierSharedKey(clientSecret);

	connect(oAuthFlow, &GoogleOAuthFlow::granted,
			this, &OAuthAuthenticatorPrivate::_q_firebaseSignIn);
	connect(oAuthFlow, &GoogleOAuthFlow::error,
			this, &OAuthAuthenticatorPrivate::_q_oAuthError);
	QObject::connect(oAuthFlow, &GoogleOAuthFlow::authorizeWithBrowser,
					 q, &OAuthAuthenticator::signInRequested);

	loadOaConfig();
}

void OAuthAuthenticatorPrivate::_q_firebaseSignIn()
{
	Q_Q(OAuthAuthenticator);
	if (aborted) {
		aborted = false;
		clearOaConfig();
		return;
	}

	qCDebug(logOAuth) << "OAuth-token granted - signing in to firebase...";
	storeOaConfig();

	SignInRequest request;
	request.setRequestUri(oAuthFlow->requestUrl());
	request.setPostBody(QStringLiteral("id_token=%1&providerId=google.com").arg(oAuthFlow->idToken()));
	const auto reply = api->oAuthSignIn(request);
	lastReply = reply->networkReply();
	reply->onSucceeded(q, [this](int, const SignInResponse &response) {
		Q_Q(OAuthAuthenticator);
		if (response.needConfirmation()) {
			qCCritical(logOAuth) << "Another account with the same credentials already exists!"
								 << "The user must log in with that account instead or link the two accounts!";
			q->failSignIn(OAuthAuthenticator::tr("Another account with the same credentials already exists! "
												 "You have to log in with that account instead or link the two accounts!"));
			return;
		}
		qCDebug(logOAuth) << "Firebase sign in successful";
		if (!response.emailVerified())
			qCWarning(logOAuth) << "Account-Mail was not verified!";
		q->completeSignIn(response.localId(),
						  response.idToken(),
						  response.refreshToken(),
						  QDateTime::currentDateTimeUtc().addSecs(response.expiresIn()),
						  response.email());
	});
	reply->onAllErrors(q, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		Q_Q(OAuthAuthenticator);
		_q_apiError(error, code, errorType);
		qCCritical(logOAuth) << "Failed to sign in to firebase with google OAuth credentials -"
							 << "make shure google OAuth authentication has been enabled in the firebase console!";
		q->failSignIn(OAuthAuthenticator::tr("Google Authentication was not accepted by firebase"));
	}, &FirebaseAuthenticatorPrivate::translateError);
}

void OAuthAuthenticatorPrivate::_q_oAuthError(const QString &error, const QString &errorDescription)
{
	Q_Q(OAuthAuthenticator);
	qCCritical(logOAuth).nospace() << "OAuth flow failed with error " << error
								   << ": " << qUtf8Printable(errorDescription);
	clearOaConfig();
	if (aborted)
		aborted = false;
	else
		q->failSignIn(OAuthAuthenticator::tr("Failed to sign in with Google-Account!"));
}

void OAuthAuthenticatorPrivate::loadOaConfig()
{
	oAuthFlow->setRefreshToken(settings->value(OAuthRefreshTokenKey).toString());
}

void OAuthAuthenticatorPrivate::storeOaConfig()
{
	settings->setValue(OAuthRefreshTokenKey, oAuthFlow->refreshToken());
}

void OAuthAuthenticatorPrivate::clearOaConfig()
{
	settings->remove(OAuthGroupKey);
}

#include "moc_authenticator.cpp"
