#include "googleauthenticator.h"
#include "googleauthenticator_p.h"
#include "firebaseauthenticator_p.h"
using namespace QtDataSync;
using namespace QtDataSync::__private;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::auth;

Q_LOGGING_CATEGORY(QtDataSync::logGAuth, "qt.datasync.GoogleAuthenticator")

GoogleAuthenticator::GoogleAuthenticator(const QString &clientId, const QString &clientSecret, quint16 callbackPort, QObject *parent) :
	OAuthAuthenticator{*new GoogleAuthenticatorPrivate{}, parent}
{
	Q_D(GoogleAuthenticator);
	d->oAuthFlow = new GoogleOAuthFlow{callbackPort, nullptr, this};
	d->oAuthFlow->setScope(QStringLiteral("openid email https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/firebase.database"));
	d->oAuthFlow->setClientIdentifier(clientId);
	d->oAuthFlow->setClientIdentifierSharedKey(clientSecret);

	QObjectPrivate::connect(d->oAuthFlow, &GoogleOAuthFlow::granted,
							d, &GoogleAuthenticatorPrivate::_q_firebaseSignIn);
	QObjectPrivate::connect(d->oAuthFlow, &GoogleOAuthFlow::error,
							d, &GoogleAuthenticatorPrivate::_q_oAuthError);
	connect(d->oAuthFlow, &GoogleOAuthFlow::authorizeWithBrowser,
			this, &GoogleAuthenticator::signInRequested);
}

bool GoogleAuthenticator::doesPreferNative() const
{
	Q_D(const GoogleAuthenticator);
	return d->preferNative;
}

void GoogleAuthenticator::setPreferNative(bool preferNative)
{
	Q_D(GoogleAuthenticator);
	if (d->preferNative == preferNative)
		return;

	d->preferNative = preferNative;
	emit preferNativeChanged(d->preferNative, {});
}

void GoogleAuthenticator::init()
{
	OAuthAuthenticator::init();
	Q_D(GoogleAuthenticator);
	d->oAuthFlow->setNetworkAccessManager(client()->manager());
	d->loadOaConfig();
}

void GoogleAuthenticator::signIn()
{
	Q_D(GoogleAuthenticator);
	if (!d->oAuthFlow->idToken().isEmpty() &&
		d->oAuthFlow->expirationAt().toUTC() > QDateTime::currentDateTimeUtc()) { // valid id token -> done
		qCDebug(logGAuth) << "OAuth-token still valid - sign in successfull";
		d->oAuthState = GoogleAuthenticatorPrivate::OAuthState::Inactive;
		d->_q_firebaseSignIn();
	} else if (!d->oAuthFlow->refreshToken().isEmpty()) { // valid refresh token -> do the refresh
		qCDebug(logGAuth) << "OAuth-token expired - refreshing...";
		d->oAuthState = GoogleAuthenticatorPrivate::OAuthState::Active;
		d->oAuthFlow->refreshAccessToken();
	} else { // no token or refresh token -> run full oauth flow
		qCDebug(logGAuth) << "OAuth-token invalid or expired - signing in...";
		d->oAuthState = GoogleAuthenticatorPrivate::OAuthState::Active;
		d->clearOaConfig();
		d->oAuthFlow->grant();
	}
}

void GoogleAuthenticator::logOut()
{
	Q_D(GoogleAuthenticator);
	d->oAuthState = GoogleAuthenticatorPrivate::OAuthState::Inactive;
	d->oAuthFlow->setIdToken({});
	d->oAuthFlow->setRefreshToken({});
	d->clearOaConfig();
}

void GoogleAuthenticator::abortRequest()
{
	OAuthAuthenticator::abortRequest();
	Q_D(GoogleAuthenticator);
	if (d->oAuthState == GoogleAuthenticatorPrivate::OAuthState::Active)
		d->oAuthState = GoogleAuthenticatorPrivate::OAuthState::Aborted;
}



const QString GoogleAuthenticatorPrivate::OAuthGroupKey = QStringLiteral("auth/google-oauth");
const QString GoogleAuthenticatorPrivate::OAuthRefreshTokenKey = QStringLiteral("auth/google-oauth/refreshToken");

void GoogleAuthenticatorPrivate::_q_firebaseSignIn()
{
	Q_Q(GoogleAuthenticator);
	if (oAuthState == OAuthState::Aborted) {
		oAuthState = OAuthState::Inactive;
		clearOaConfig();
		q->failSignIn();
		return;
	} else
		oAuthState = OAuthState::Inactive;

	storeOaConfig();
	q->signInWithToken(oAuthFlow->requestUrl(),
					   QStringLiteral("google.com"),
					   oAuthFlow->idToken());
}

void GoogleAuthenticatorPrivate::_q_oAuthError(const QString &error, const QString &errorDescription)
{
	Q_Q(GoogleAuthenticator);
	qCCritical(logGAuth).nospace() << "OAuth flow failed with error " << error
								   << ": " << qUtf8Printable(errorDescription);
	clearOaConfig();
	if (oAuthState == OAuthState::Aborted)
		q->failSignIn();
	else
		Q_EMIT q->guiError(GoogleAuthenticator::tr("Failed to sign in with Google-Account!"));
	oAuthState = OAuthState::Inactive;
}

void GoogleAuthenticatorPrivate::loadOaConfig()
{
	Q_Q(GoogleAuthenticator);
	oAuthFlow->setRefreshToken(q->settings()->value(OAuthRefreshTokenKey).toString());
}

void GoogleAuthenticatorPrivate::storeOaConfig()
{
	Q_Q(GoogleAuthenticator);
	q->settings()->setValue(OAuthRefreshTokenKey, oAuthFlow->refreshToken());
}

void GoogleAuthenticatorPrivate::clearOaConfig()
{
	Q_Q(GoogleAuthenticator);
	q->settings()->remove(OAuthGroupKey);
}



void GoogleAuthenticatorExtensionPrivate::extendFromWebConfig(const QJsonObject &config)
{
	const auto jAuth = config[QStringLiteral("oAuth")].toObject();
	clientId = jAuth[QStringLiteral("clientID")].toString();
	secret = jAuth[QStringLiteral("clientSecret")].toString();
	port = static_cast<quint16>(jAuth[QStringLiteral("callbackPort")].toInt());
}

void GoogleAuthenticatorExtensionPrivate::extendFromGSJsonConfig(const QJsonObject &config)
{
	const auto clients = config[QStringLiteral("client")].toArray();
	for (const auto &clientV : clients) {
		const auto client = clientV.toObject();
		const auto oauth_clients = client[QStringLiteral("oauth_client")].toArray();
		for (const auto &oauth_clientV : oauth_clients) {
			const auto oauth_client = oauth_clientV.toObject();
			clientId = oauth_client[QStringLiteral("client_id")].toString();
			secret = oauth_client[QStringLiteral("client_secret")].toString();
			port = static_cast<quint16>(oauth_client[QStringLiteral("callback_port")].toInt());
			if (!clientId.isEmpty())
				return;
		}
	}
}

void GoogleAuthenticatorExtensionPrivate::extendFromGSPlistConfig(QSettings *config)
{
	clientId = config->value(QStringLiteral("CLIENT_ID")).toString();
	secret = config->value(QStringLiteral("CLIENT_SECRET")).toString();
	port = static_cast<quint16>(config->value(QStringLiteral("CALLBACK_PORT")).toInt());
}

void GoogleAuthenticatorExtensionPrivate::testConfigSatisfied(const QLoggingCategory &logCat)
{
	if (clientId.isEmpty())
		qCWarning(logCat).noquote() << "Unable to find the Google OAuth client-ID in the configuration file";
	else {
		if (clientId.isEmpty())
			qCWarning(logCat).noquote() << "Unable to find the Google OAuth client-secret in the configuration file";
		if (port == 0)
			qCWarning(logCat).noquote() << "Unable to find the Google OAuth callback-port in the configuration file";
	}
}

QObject *GoogleAuthenticatorExtensionPrivate::createInstance(QObject *parent)
{
	return new GoogleAuthenticator {
		clientId,
		secret,
		port,
		parent
	};
}

#include "moc_googleauthenticator.cpp"
