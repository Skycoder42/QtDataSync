#include "authenticator.h"
#include "authenticator_p.h"
#include "engine_p.h"

#include <random>

#include <QtCore/QRandomGenerator>
#include <QtCore/QCryptographicHash>

#include <QtNetworkAuth/QOAuthHttpServerReplyHandler>
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::auth;

Q_LOGGING_CATEGORY(QtDataSync::logFbAuth, "qt.datasync.FirebaseAuthenticator")
Q_LOGGING_CATEGORY(QtDataSync::logOAuth, "qt.datasync.OAuthAuthenticator")

IAuthenticator::IAuthenticator(QObject *parent) :
	QObject{parent}
{}

IAuthenticator::IAuthenticator(QObjectPrivate &dd, QObject *parent) :
	QObject{dd, parent}
{}



void FirebaseAuthenticator::signIn()
{
	Q_D(FirebaseAuthenticator);
	if (!d->idToken.isEmpty() && d->expiresAt > QDateTime::currentDateTimeUtc()) { // token valid and not expired -> done
		qCDebug(logFbAuth) << "Firebase-token still valid - sign in successfull";
		Q_EMIT signInSuccessful(d->localId, d->idToken);
	} else if (!d->refreshToken.isEmpty()) { // has refresh token -> refresh
		qCDebug(logFbAuth) << "Firebase-token expired - refreshing...";
		d->api->refreshToken(d->refreshToken)->onSucceeded([this](int, const RefreshResponse &response) {
			Q_D(FirebaseAuthenticator);
			if (response.token_type() != QStringLiteral("Bearer")) {
				qCCritical(logFbAuth) << "Invalid token type" << response.token_type();
				Q_EMIT signInFailed();
				return;
			}
			d->localId = response.user_id();
			d->idToken = response.id_token();
			d->refreshToken = response.refresh_token();
			d->expiresAt = QDateTime::currentDateTimeUtc().addSecs(response.expires_in());
			qCDebug(logFbAuth) << "Firebase-token refresh successful";
			Q_EMIT signInSuccessful(d->localId, d->idToken);
		})->onAllErrors(this, [this](const QString &, int, QtRestClient::RestReply::Error){
			qCWarning(logFbAuth) << "Refreshing the firebase-token failed - signing in again...";
			firebaseSignIn();
		});
	} else { // no token and no refresh -> normal sign in
		qCDebug(logFbAuth) << "Firebase-token invalid or expired - signing in...";
		firebaseSignIn();
	}
}

void FirebaseAuthenticator::deleteUser()
{
	Q_D(FirebaseAuthenticator);
	if (d->idToken.isEmpty()) {
		qCWarning(logFbAuth) << "Cannot delete account without beeing logged in";
		Q_EMIT accountDeleted(false);
		return;
	}

	d->api->deleteAccount(d->idToken)->onSucceeded(this, [this](int) {
		qCDebug(logFbAuth) << "Firebase account delete successful";
		Q_EMIT accountDeleted(true);
	})->onAllErrors(this, [this](const QString &, int, QtRestClient::RestReply::Error){
		qCWarning(logFbAuth) << "Firebase account delete failed";
		Q_EMIT accountDeleted(false);
	});
}

FirebaseAuthenticator::FirebaseAuthenticator(Engine *engine) :
	FirebaseAuthenticator{*new FirebaseAuthenticatorPrivate{}, engine}
{}

FirebaseAuthenticator::FirebaseAuthenticator(FirebaseAuthenticatorPrivate &dd, Engine *engine) :
	IAuthenticator{dd, engine}
{
	Q_D(FirebaseAuthenticator);
	// setup rest api
	auto client = new QtRestClient::RestClient{QtRestClient::RestClient::DataMode::Json, this};
	client->setModernAttributes();
	client->setBaseUrl(QUrl{QStringLiteral("https://identitytoolkit.googleapis.com")});
	client->setApiVersion(QVersionNumber{1});
	client->addGlobalParameter(QStringLiteral("key"), EnginePrivate::setupFor(engine)->firebase.webApiKey);
	d->api = new ApiClient{client->rootClass(), this};
	d->api->setErrorTranslator(&FirebaseAuthenticatorPrivate::translateError);
	QObjectPrivate::connect(d->api, &ApiClient::apiError,
							d, &FirebaseAuthenticatorPrivate::_q_apiError);
}

void FirebaseAuthenticator::completeSignIn(QString localId, QString idToken, QString refreshToken, const QDateTime &expiresAt, QString email)
{
	Q_D(FirebaseAuthenticator);
	d->localId = std::move(localId);
	d->idToken = std::move(idToken);
	d->refreshToken = std::move(refreshToken);
	d->expiresAt = expiresAt.toUTC();
	d->email = std::move(email);
	Q_EMIT signInSuccessful(d->localId, d->idToken);
}



QString FirebaseAuthenticatorPrivate::translateError(const FirebaseError &error, int code)
{
	if (const auto msg = error.error().message(); !msg.isEmpty())
		return msg;
	for (const auto &subMsg : error.error().errors()) {
		if (const auto msg = subMsg.message(); !msg.isEmpty())
			return msg;
	}
	return QStringLiteral("Request failed with status code %1").arg(code);
}

void FirebaseAuthenticatorPrivate::_q_apiError(const QString &errorString, int errorCode, QtRestClient::RestReply::Error errorType)
{
	qCWarning(logFbAuth) << "API request failed with reason" << errorType
						 << "and code" << errorCode
						 << "- error message:" << qUtf8Printable(errorString);
}




OAuthAuthenticator::OAuthAuthenticator(Engine *engine) :
	FirebaseAuthenticator{*new OAuthAuthenticatorPrivate{}, engine}
{
	Q_D(OAuthAuthenticator);
	const auto setup = EnginePrivate::setupFor(engine);
	d->oAuthFlow = new GoogleOAuthFlow{setup->oAuth.port, d->api->restClient()->manager(), this};
	d->oAuthFlow->setScope(QStringLiteral("openid email https://www.googleapis.com/auth/datastore https://www.googleapis.com/auth/cloud-platform"));
	d->oAuthFlow->setClientIdentifier(setup->oAuth.clientId);
	d->oAuthFlow->setClientIdentifierSharedKey(setup->oAuth.secret);

	QObjectPrivate::connect(d->oAuthFlow, &GoogleOAuthFlow::granted,
							d, &OAuthAuthenticatorPrivate::_q_firebaseSignIn);
	QObjectPrivate::connect(d->oAuthFlow, &GoogleOAuthFlow::error,
							d, &OAuthAuthenticatorPrivate::_q_oAuthError);
	connect(d->oAuthFlow, &GoogleOAuthFlow::authorizeWithBrowser,
			this, &OAuthAuthenticator::signInRequested);
}

bool OAuthAuthenticator::doesPreferNative() const
{
	Q_D(const OAuthAuthenticator);
	return d->preferNative;
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
	if (!d->oAuthFlow->idToken().isEmpty() && d->oAuthFlow->expirationAt().toUTC() > QDateTime::currentDateTimeUtc()) { // valid id token -> done
		qCDebug(logOAuth) << "OAuth-token still valid - sign in successfull";
		d->_q_firebaseSignIn();
	} else if (!d->oAuthFlow->refreshToken().isEmpty()) { // valid refresh token -> do the refresh
		qCDebug(logOAuth) << "OAuth-token expired - refreshing...";
		d->oAuthFlow->refreshAccessToken();
	} else { // no token or refresh token -> run full oauth flow
		qCDebug(logOAuth) << "OAuth-token invalid or expired - signing in...";
		d->oAuthFlow->grant();
	}
}



void OAuthAuthenticatorPrivate::_q_firebaseSignIn()
{
	Q_Q(OAuthAuthenticator);
	qCDebug(logOAuth) << "OAuth-token granted - signing in to firebase...";
	SignInRequest request;
	request.setRequestUri(oAuthFlow->requestUrl());
	request.setPostBody(QStringLiteral("id_token=%1&providerId=google.com").arg(oAuthFlow->idToken()));
	api->oAuthSignIn(request)->onSucceeded(q, [this](int, const SignInResponse &response) {
		Q_Q(OAuthAuthenticator);
		if (response.needConfirmation()) {
			qCCritical(logOAuth) << "Another account with the same credentials already exists!"
								 << "The user must log in with that account instead or link the two accounts!";
			Q_EMIT q->signInFailed();
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

}

void OAuthAuthenticatorPrivate::_q_oAuthError(const QString &error, const QString &errorDescription)
{
	Q_Q(OAuthAuthenticator);
	qCCritical(logOAuth).nospace() << "OAuth flow failed with error " << error
								   << ": " << qUtf8Printable(errorDescription);
	Q_EMIT q->signInFailed();
}



GoogleOAuthFlow::GoogleOAuthFlow(quint16 port, QNetworkAccessManager *nam, QObject *parent) :
	QOAuth2AuthorizationCodeFlow{nam, parent}
{
	_handler = new QOAuthHttpServerReplyHandler{port, this};
	connect(_handler, &QAbstractOAuthReplyHandler::tokensReceived,
			this, &GoogleOAuthFlow::handleIdToken);
	setReplyHandler(_handler);

	setAuthorizationUrl(QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth"));
	setAccessTokenUrl(QStringLiteral("https://www.googleapis.com/oauth2/v4/token"));
	setModifyParametersFunction([this](QAbstractOAuth::Stage stage, QVariantMap *parameters) {
		switch (stage) {
		case QAbstractOAuth::Stage::RequestingAuthorization:
			// add parameters
			parameters->insert(QStringLiteral("prompt"), QStringLiteral("select_account"));
			parameters->insert(QStringLiteral("access_type"), QStringLiteral("offline"));
			parameters->insert(QStringLiteral("code_challenge_method"), QStringLiteral("S256"));
			parameters->insert(QStringLiteral("code_challenge"), createChallenge());
			break;
		case QAbstractOAuth::Stage::RequestingAccessToken:
			parameters->insert(QStringLiteral("code_verifier"), _codeVerifier);
			urlUnescape(parameters, QStringLiteral("code"));
			urlUnescape(parameters, QStringLiteral("redirect_uri"));
			break;
		default:
			break;
		}
	});
}

QString GoogleOAuthFlow::idToken() const
{
	return _idToken;
}

QUrl GoogleOAuthFlow::requestUrl() const
{
	return _handler->callback();
}

void GoogleOAuthFlow::handleIdToken(const QVariantMap &values)
{
	_idToken = values.value(QStringLiteral("id_token")).toString();
}

QString GoogleOAuthFlow::createChallenge()
{
	if (!_challengeEngine)
		_challengeEngine = BitEngine{QRandomGenerator::securelySeeded()};

	QByteArray data(96, 0);
	std::generate(data.begin(), data.end(), *_challengeEngine);
	const auto verifierData = data.toBase64(QByteArray::Base64UrlEncoding);
	_codeVerifier = QString::fromUtf8(verifierData);

	return QString::fromUtf8(
		QCryptographicHash::hash(verifierData, QCryptographicHash::Sha256)
			.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

void GoogleOAuthFlow::urlUnescape(QVariantMap *parameters, const QString &key) const
{
	parameters->insert(key, QUrl::fromPercentEncoding(parameters->value(key).toString().toUtf8()));
}

#include "moc_authenticator.cpp"
