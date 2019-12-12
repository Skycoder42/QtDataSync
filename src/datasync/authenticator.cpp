#include "authenticator.h"
#include "authenticator_p.h"

#include <random>

#include <QtCore/QRandomGenerator>
#include <QtCore/QCryptographicHash>

#include <QtNetworkAuth/QOAuthHttpServerReplyHandler>
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::auth;

Q_LOGGING_CATEGORY(QtDataSync::logAuth, "qt.datasync.OAuthAuthenticator")

IAuthenticator::IAuthenticator(QObject *parent) :
	QObject{parent}
{}

IAuthenticator::IAuthenticator(QObjectPrivate &dd, QObject *parent) :
	QObject{dd, parent}
{}



OAuthAuthenticator::OAuthAuthenticator(const QString &webApiKey, Engine *engine) :
	IAuthenticator{*new OAuthAuthenticatorPrivate{}, engine}
{
	Q_D(OAuthAuthenticator);

	// setup rest api
	auto client = new QtRestClient::RestClient{QtRestClient::RestClient::DataMode::Json, this};
	client->setBaseUrl(QUrl{QStringLiteral("https://identitytoolkit.googleapis.com")});
	client->setApiVersion(QVersionNumber{1});
	client->addGlobalParameter(QStringLiteral("key"), webApiKey);
	d->api = new ApiClient{client->rootClass(), this};
	d->api->setErrorTranslator(&OAuthAuthenticatorPrivate::translateError);
}

bool OAuthAuthenticator::doesPreferNative() const
{
	Q_D(const OAuthAuthenticator);
	return d->preferNative;
}

void OAuthAuthenticator::signIn()
{
	Q_D(OAuthAuthenticator);

	// first: check if user already logged in with valid token -> done
	if (d->expireDate > QDateTime::currentDateTimeUtc()) {
		Q_EMIT signInSuccessful(d->localId, d->idToken);
		return;
	}

	// second: if a refresh-token is available, try to refesh
	if (!d->refreshToken.isEmpty()) {
		d->api->refreshToken(d->refreshToken)->onSucceeded(this, [this](int, const RefreshResponse &response) {
			Q_D(OAuthAuthenticator);
			if (response.token_type() != QStringLiteral("Bearer")) {
				// TODO log error
				Q_EMIT signInFailed();
				return;
			}
			d->localId = response.user_id();
			d->idToken = response.id_token();
			d->refreshToken = response.refresh_token();
			d->expireDate = QDateTime::currentDateTimeUtc().addSecs(response.expires_in());
			Q_EMIT signInSuccessful(d->localId, d->idToken);
		});
		return;
	}

	// third: do a normal OAuth flow -> request OAuth from user
	d->runOAuth();
}

void OAuthAuthenticator::deleteAccount()
{
}

void OAuthAuthenticator::setPreferNative(bool preferNative)
{
	Q_D(OAuthAuthenticator);
	if (d->preferNative == preferNative)
		return;

	d->preferNative = preferNative;
	emit preferNativeChanged(d->preferNative, {});
}

QString OAuthAuthenticatorPrivate::translateError(const FirebaseError &error, int)
{
	return error.error().message();
}

void OAuthAuthenticatorPrivate::initOAuth()
{
	Q_Q(OAuthAuthenticator);
	if (oAuthFlow)
		return;

	oAuthFlow = new IdTokenFlow{35634, q};
	oAuthFlow->setAuthorizationUrl(QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth"));
	oAuthFlow->setAccessTokenUrl(QStringLiteral("https://www.googleapis.com/oauth2/v4/token"));
	oAuthFlow->setScope(QStringLiteral("openid email https://www.googleapis.com/auth/datastore https://www.googleapis.com/auth/cloud-platform"));
	oAuthFlow->setClientIdentifier(QStringLiteral("***"));
	oAuthFlow->setClientIdentifierSharedKey(QStringLiteral("***"));
	oAuthFlow->setModifyParametersFunction([this](QAbstractOAuth::Stage stage, QVariantMap *parameters) {
		switch (stage) {
		case QAbstractOAuth::Stage::RequestingAuthorization:
			// add parameters
			parameters->insert(QStringLiteral("prompt"), QStringLiteral("select_account"));
			parameters->insert(QStringLiteral("access_type"), QStringLiteral("offline"));
			parameters->insert(QStringLiteral("code_challenge_method"), QStringLiteral("S256"));
			parameters->insert(QStringLiteral("code_challenge"), createChallenge());
			break;
		case QAbstractOAuth::Stage::RequestingAccessToken:
			parameters->insert(QStringLiteral("code_verifier"), codeVerifier);
			urlUnescape(parameters, QStringLiteral("code"));
			urlUnescape(parameters, QStringLiteral("redirect_uri"));
			break;
		default:
			break;
		}
	});

	QObject::connect(oAuthFlow, &QOAuth2AuthorizationCodeFlow::statusChanged,
		q, [this](QAbstractOAuth::Status status) {
			if (status == QAbstractOAuth::Status::Granted)
				firebaseAuth();
		});
	QObject::connect(oAuthFlow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
					 q, &OAuthAuthenticator::signInRequested);
}

void OAuthAuthenticatorPrivate::runOAuth()
{
	initOAuth();
	if (oAuthFlow->refreshToken().isEmpty() || oAuthFlow->idToken().isEmpty())  // no token -> login
		oAuthFlow->grant();
	else if (oAuthFlow->expirationAt().toUTC() <= QDateTime::currentDateTimeUtc())  // token expired -> refresh
		oAuthFlow->refreshAccessToken();
	else  // token valid -> continue to firebase
		firebaseAuth();
}

void OAuthAuthenticatorPrivate::firebaseAuth()
{
	qCDebug(logAuth) << oAuthFlow->idToken()
					 << oAuthFlow->refreshToken()
					 << oAuthFlow->expirationAt();
}

QString OAuthAuthenticatorPrivate::createChallenge()
{
	if (!challengeEngine)
		challengeEngine = BitEngine{QRandomGenerator::securelySeeded()};

	QByteArray data(96, 0);
	std::generate(data.begin(), data.end(), *challengeEngine);
	const auto verifierData = data.toBase64(QByteArray::Base64UrlEncoding);
	codeVerifier = QString::fromUtf8(verifierData);

	return QString::fromUtf8(
		QCryptographicHash::hash(verifierData, QCryptographicHash::Sha256)
				.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

void OAuthAuthenticatorPrivate::urlUnescape(QVariantMap *parameters, const QString &key) const
{
	parameters->insert(key, QUrl::fromPercentEncoding(parameters->value(key).toString().toUtf8()));
}



IdTokenFlow::IdTokenFlow(quint16 port, QObject *parent) :
	QOAuth2AuthorizationCodeFlow{parent}
{
	auto handler = new QOAuthHttpServerReplyHandler{port, this};
	connect(handler, &QAbstractOAuthReplyHandler::tokensReceived,
			this, &IdTokenFlow::handleIdToken);
	setReplyHandler(handler);
}

QString IdTokenFlow::idToken() const
{
	return _idToken;
}

void IdTokenFlow::handleIdToken(const QVariantMap &values)
{
	_idToken = values.value(QStringLiteral("id_token")).toString();
}
