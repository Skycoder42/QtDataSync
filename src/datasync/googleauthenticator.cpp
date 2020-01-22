#include "googleauthenticator.h"
#include "googleauthenticator_p.h"
#include "firebaseauthenticator_p.h"
using namespace QtDataSync;
using namespace QtDataSync::__private;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::auth;

Q_LOGGING_CATEGORY(QtDataSync::logOAuth, "qt.datasync.GoogleAuthenticator")

GoogleAuthenticator::GoogleAuthenticator(const QString &clientId, const QString &clientSecret, quint16 callbackPort, QObject *parent) :
	IAuthenticator{*new GoogleAuthenticatorPrivate{}, parent}
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

void GoogleAuthenticator::signIn()
{
	Q_D(GoogleAuthenticator);
	d->aborted = false;
	if (!d->oAuthFlow->idToken().isEmpty() &&
		d->oAuthFlow->expirationAt().toUTC() > QDateTime::currentDateTimeUtc()) { // valid id token -> done
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

void GoogleAuthenticator::logOut()
{
	Q_D(GoogleAuthenticator);
	d->oAuthFlow->setIdToken({});
	d->oAuthFlow->setRefreshToken({});
	d->clearOaConfig();
}

void GoogleAuthenticator::abortRequest()
{
	Q_D(GoogleAuthenticator);
	if (d->lastReply)
		d->lastReply->abort();
	else {
		d->aborted = true;
		failSignIn(tr("Sign-In aborted!"));  // TODO only if running
	}
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
	Q_D(GoogleAuthenticator);
	d->oAuthFlow->setNetworkAccessManager(client()->manager());
	d->api = new ApiClient{client()->rootClass(), this};
	d->loadOaConfig();
}



const QString GoogleAuthenticatorPrivate::OAuthGroupKey = QStringLiteral("auth/google-oauth");
const QString GoogleAuthenticatorPrivate::OAuthRefreshTokenKey = QStringLiteral("auth/google-oauth/refreshToken");

void GoogleAuthenticatorPrivate::_q_firebaseSignIn()
{
	Q_Q(GoogleAuthenticator);
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
		Q_Q(GoogleAuthenticator);
		if (response.needConfirmation()) {
			qCCritical(logOAuth) << "Another account with the same credentials already exists!"
								 << "The user must log in with that account instead or link the two accounts!";
			q->failSignIn(GoogleAuthenticator::tr("Another account with the same credentials already exists! "
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
		Q_Q(GoogleAuthenticator);
		FirebaseAuthenticator::logError(error, code, errorType);
		qCCritical(logOAuth) << "Failed to sign in to firebase with google OAuth credentials -"
							 << "make shure google OAuth authentication has been enabled in the firebase console!";
		q->failSignIn(GoogleAuthenticator::tr("Google Authentication was not accepted by firebase"));
	}, &FirebaseAuthenticator::translateError);
}

void GoogleAuthenticatorPrivate::_q_oAuthError(const QString &error, const QString &errorDescription)
{
	Q_Q(GoogleAuthenticator);
	qCCritical(logOAuth).nospace() << "OAuth flow failed with error " << error
								   << ": " << qUtf8Printable(errorDescription);
	clearOaConfig();
	if (aborted)
		aborted = false;
	else
		Q_EMIT q->guiError(GoogleAuthenticator::tr("Failed to sign in with Google-Account!"));
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
