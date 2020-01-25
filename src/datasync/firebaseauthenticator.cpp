#include "firebaseauthenticator_p.h"
#include "authenticator_p.h"
#include <QtCore/QAtomicInteger>
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::auth;
using namespace std::chrono;
using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(QtDataSync::logFbAuth, "qt.datasync.FirebaseAuthenticator")

const QString FirebaseAuthenticator::FirebaseGroupKey = QStringLiteral("auth/firebase");
const QString FirebaseAuthenticator::FirebaseRefreshTokenKey = QStringLiteral("auth/firebase/refreshToken");
const QString FirebaseAuthenticator::FirebaseEmailKey = QStringLiteral("auth/firebase/email");

void FirebaseAuthenticator::logError(const QString &errorString, int errorCode, QtRestClient::RestReply::Error errorType)
{
	qCWarning(logFbAuth) << "API request failed with reason" << errorType
						 << "and code" << errorCode
						 << "- error message:" << qUtf8Printable(errorString);
}

QString FirebaseAuthenticator::translateError(const Error &error, int code)
{
	if (const auto msg = error.error().message(); !msg.isEmpty())
		return msg;
	for (const auto &subMsg : error.error().errors()) {
		if (const auto msg = subMsg.message(); !msg.isEmpty())
			return msg;
	}
	return QStringLiteral("Request failed with status code %1").arg(code);
}

FirebaseAuthenticator::FirebaseAuthenticator(IAuthenticator *authenticator, const QString &apiKey, QSettings *settings, QNetworkAccessManager *nam, QObject *parent) :
	QObject{parent},
	_auth{authenticator},
	_settings{settings},
	_refreshTimer{new QTimer{this}}
{
	// registrations
#ifdef Q_ATOMIC_INT8_IS_SUPPORTED
	static QAtomicInteger<bool> authReg = false;
#else
	static QAtomicInteger<quint16> authReg = false;
#endif
	if (authReg.testAndSetOrdered(false, true)) {
		qRegisterMetaType<ErrorContent>("ErrorContent");
		QtJsonSerializer::SerializerBase::registerListConverters<ErrorContent>();
	}

	// timer
	_refreshTimer->setSingleShot(true);
	_refreshTimer->setTimerType(Qt::VeryCoarseTimer);
	connect(_refreshTimer, &QTimer::timeout,
			this, &FirebaseAuthenticator::refreshToken);

	// api
	auto client = new QtRestClient::RestClient{QtRestClient::RestClient::DataMode::Json, this};
	client->setManager(nam);
	client->setModernAttributes();
	client->addRequestAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
	client->setBaseUrl(QUrl{QStringLiteral("https://identitytoolkit.googleapis.com")});
	client->setApiVersion(QVersionNumber{1});
	client->addGlobalParameter(QStringLiteral("key"), apiKey);
	_api = new ApiClient{client->rootClass(), this};
	client->setParent(_api);
	loadFbConfig();

	// initialize auth
	_auth->init(this);
}

IAuthenticator *FirebaseAuthenticator::authenticator() const
{
	return _auth;
}

void FirebaseAuthenticator::signIn()
{
	if (!_idToken.isEmpty() && _expiresAt > QDateTime::currentDateTimeUtc()) { // token valid and not expired -> done
		qCDebug(logFbAuth) << "Firebase-token still valid - sign in successfull";
		Q_EMIT signInSuccessful(_localId, _idToken);
	} else if (!_refreshToken.isEmpty()) // has refresh token -> refresh
		refreshToken();
	else { // no token and no refresh -> normal sign in
		qCDebug(logFbAuth) << "Firebase-token invalid or expired - signing in...";
		clearFbConfig();
		_auth->signIn();
	}
}

void FirebaseAuthenticator::logOut()
{
	_refreshTimer->stop();
	_localId.clear();
	_idToken.clear();
	_refreshToken.clear();
	_email.clear();
	clearFbConfig();

	_auth->logOut();
}

void FirebaseAuthenticator::deleteUser()
{
	if (_idToken.isEmpty()) {
		qCWarning(logFbAuth) << "Cannot delete account without beeing logged in";
		logOut();
		Q_EMIT accountDeleted(false);
		return;
	}

	const auto reply = _api->deleteAccount(_idToken);
	_lastReply = reply->networkReply();
	reply->onSucceeded(this, [this](int) {
		qCDebug(logFbAuth) << "Firebase account delete successful";
		logOut();
		Q_EMIT accountDeleted(true);
	});
	reply->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType){
		logError(error, code, errorType);
		qCWarning(logFbAuth) << "Firebase account delete failed";
		Q_EMIT accountDeleted(false);
	}, &FirebaseAuthenticator::translateError);
}

void FirebaseAuthenticator::abortRequest()
{
	if (_lastReply)
		_lastReply->abort();
	_auth->abortRequest();
}

void FirebaseAuthenticator::refreshToken()
{
	qCDebug(logFbAuth) << "Firebase-token expired - refreshing...";
	const auto reply = _api->refreshToken(_refreshToken);
	_lastReply = reply->networkReply();
	reply->onSucceeded(this, [this](int, const RefreshResponse &response) {
		if (response.token_type() != QStringLiteral("Bearer")) {
			qCCritical(logFbAuth) << "Invalid token type" << response.token_type();
			Q_EMIT signInFailed();
			return;
		}
		qCDebug(logFbAuth) << "Firebase-token refresh successful";
		_localId = response.user_id();
		_idToken = response.id_token();
		_refreshToken = response.refresh_token();
		_expiresAt = QDateTime::currentDateTimeUtc().addSecs(response.expires_in());
		storeFbConfig();
		startRefreshTimer();
		Q_EMIT signInSuccessful(_localId, _idToken);
	});
	reply->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType){
		logError(error, code, errorType);
		qCWarning(logFbAuth) << "Refreshing the firebase-token failed - signing in again...";
		clearFbConfig();
		_auth->signIn();
	}, &FirebaseAuthenticator::translateError);
}

void FirebaseAuthenticator::startRefreshTimer()
{
	// schedule a refresh, one minute early
	auto expireDelta = milliseconds{QDateTime::currentDateTimeUtc().msecsTo(_expiresAt)} - 1min;
	if (expireDelta <= 0ms)
		refreshToken();
	else {
		_refreshTimer->start(expireDelta);
		qCDebug(logFbAuth) << "Scheduled token refresh in"
						   << expireDelta.count()
						   << "ms";
	}
}

void FirebaseAuthenticator::loadFbConfig()
{
	_refreshToken = _settings->value(FirebaseRefreshTokenKey).toString();
	_email = _settings->value(FirebaseEmailKey).toString();
}

void FirebaseAuthenticator::storeFbConfig()
{
	_settings->setValue(FirebaseRefreshTokenKey, _refreshToken);
	_settings->setValue(FirebaseEmailKey, _email);
}

void FirebaseAuthenticator::clearFbConfig()
{
	_settings->remove(FirebaseGroupKey);
}
