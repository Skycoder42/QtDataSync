#include "mailauthenticator.h"
#include "mailauthenticator_p.h"
#include "firebaseauthenticator_p.h"
#include <QtNetwork/qpassworddigestor.h>
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::auth;

Q_LOGGING_CATEGORY(QtDataSync::logMailAuth, "qt.datasync.MailAuthenticator")

const QString MailAuthenticatorPrivate::RememberMailKey = QStringLiteral("auth/mail/remember");
const QString MailAuthenticatorPrivate::MailKey = QStringLiteral("auth/mail/mail");

MailAuthenticator::MailAuthenticator(QObject *parent) :
	IAuthenticator{*new MailAuthenticatorPrivate{}, parent}
{}

QCryptographicHash::Algorithm MailAuthenticator::hash() const
{
	Q_D(const MailAuthenticator);
	return d->hash;
}

bool MailAuthenticator::rememberMail() const
{
	Q_D(const MailAuthenticator);
	return d->rememberMail;
}

bool MailAuthenticator::requireVerification() const
{
	Q_D(const MailAuthenticator);
	return d->requireVerification;
}

void MailAuthenticator::signIn()
{
	Q_D(const MailAuthenticator);
	Q_EMIT showSignInDialog(d->rememberMail ?
								settings()->value(MailAuthenticatorPrivate::MailKey).toString() :
								QString{});
}

void MailAuthenticator::logOut()
{
	Q_D(MailAuthenticator);
	d->cachedSignUp.reset();
	d->cachedExpiresAt = {};
}

void MailAuthenticator::abortRequest()
{
	Q_D(const MailAuthenticator);
	if (d->lastReply)
		d->lastReply->abort();
}

void MailAuthenticator::signUp(const QString &mail, const QString &password)
{
	Q_D(MailAuthenticator);
	MailAuthRequest request;
	request.setEmail(mail);
	request.setPassword(d->pwHash(password));
	const auto reply = d->api->mailSignUp(request);
	d->lastReply = reply->networkReply();
	reply->onSucceeded(this, [this](int, SignInResponse response){
		Q_D(MailAuthenticator);
		d->cachedSignUp = std::move(response);
		d->cachedExpiresAt = QDateTime::currentDateTimeUtc().addSecs(d->cachedSignUp->expiresIn());
		d->sendCode();
	});
	reply->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		FirebaseAuthenticator::logError(error, code, errorType);
		qCCritical(logMailAuth) << "Failed to create email based firebase account!";
		Q_EMIT guiError(tr("Sign-Up was rejected. Either not a valid email address or it is already in use!"));
	}, &FirebaseAuthenticator::translateError);
}

void MailAuthenticator::signIn(const QString &mail, const QString &password)
{
	Q_D(MailAuthenticator);
	MailAuthRequest request;
	request.setEmail(mail);
	request.setPassword(d->pwHash(password));
	const auto reply = d->api->mailSignIn(request);
	d->lastReply = reply->networkReply();
	reply->onSucceeded(this, [this](int, const SignInResponse &response){
		completeSignIn(response.localId(),
					   response.idToken(),
					   response.refreshToken(),
					   QDateTime::currentDateTimeUtc().addSecs(response.expiresIn()),
					   response.email());
	});
	reply->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		FirebaseAuthenticator::logError(error, code, errorType);
		qCCritical(logMailAuth) << "Failed to login with email based firebase account!";
		Q_EMIT guiError(tr("Sign-In was rejected. Invalid email or wrong password!"));
	}, &FirebaseAuthenticator::translateError);
}

void MailAuthenticator::verify(const QString &code)
{
	Q_D(MailAuthenticator);
	OobCodeRequest request;
	request.setOobCode(code);
	const auto reply = d->api->mailDoVerify(request);
	d->lastReply = reply->networkReply();
	reply->onSucceeded(this, [this](int) {
		Q_D(MailAuthenticator);
		if (!d->completeSignIn(false))
			Q_EMIT codeVerified();
	});
	reply->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		FirebaseAuthenticator::logError(error, code, errorType);
		qCCritical(logMailAuth) << "Email verification code invalid!";
		Q_EMIT guiError(tr("Invalid verification code!"));
	}, &FirebaseAuthenticator::translateError);
}

void MailAuthenticator::reSendVerify()
{
	Q_D(MailAuthenticator);
	d->sendCode();
}

void MailAuthenticator::resetPassword(const QString &mail)
{
	Q_D(MailAuthenticator);
	MailResetData request;
	request.setEmail(mail);
	const auto reply = d->api->mailRequestReset(request);
	d->lastReply = reply->networkReply();
	// only handle errors
	reply->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		FirebaseAuthenticator::logError(error, code, errorType);
		qCCritical(logMailAuth) << "Failed to send email reset!";
		Q_EMIT guiError(tr("Invalid email - could not send passwort reset code!"));
	}, &FirebaseAuthenticator::translateError);
}

void MailAuthenticator::confirmReset(const QString &code, const QString &password)
{
	Q_D(MailAuthenticator);
	MailApplyResetRequest request;
	request.setOobCode(code);
	request.setNewPassword(d->pwHash(password));
	const auto reply = d->api->mailApplyReset(request);
	d->lastReply = reply->networkReply();
	reply->onSucceeded(this, [this](int, const MailResetData &response) {
		Q_EMIT showSignInDialog(response.email());
	});
	reply->onAllErrors(this, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		FirebaseAuthenticator::logError(error, code, errorType);
		qCCritical(logMailAuth) << "Failed to reset password!";
		Q_EMIT guiError(tr("Invalid code - password was not reset!"));
	}, &FirebaseAuthenticator::translateError);
}

void MailAuthenticator::setHash(QCryptographicHash::Algorithm hash)
{
	Q_D(MailAuthenticator);
	if (hash == d->hash)
		return;

	d->hash = hash;
	Q_EMIT hashChanged(d->hash);
}

void MailAuthenticator::setRememberMail(bool rememberMail)
{
	Q_D(MailAuthenticator);
	if (rememberMail == d->rememberMail)
		return;

	d->rememberMail = rememberMail;
	Q_EMIT rememberMailChanged(d->rememberMail);
}

void MailAuthenticator::setRequireVerification(bool requireVerification)
{
	Q_D(MailAuthenticator);
	if (requireVerification == d->requireVerification)
		return;

	d->requireVerification = requireVerification;
	Q_EMIT requireVerificationChanged(d->requireVerification);
}

void MailAuthenticator::init()
{
	Q_D(MailAuthenticator);
	d->rememberMail = settings()->value(MailAuthenticatorPrivate::RememberMailKey,
										d->rememberMail)
						  .toBool();
	d->api = new ApiClient{client()->rootClass(), this};
}

QString MailAuthenticatorPrivate::pwHash(const QString &password) const
{
	return QString::fromUtf8(
		QCryptographicHash::hash(password.toUtf8(),
								 hash)
			.toBase64(QByteArray::Base64Encoding | QByteArray::KeepTrailingEquals));
}

void MailAuthenticatorPrivate::sendCode()
{
	Q_Q(MailAuthenticator);

	MailVerifyRequest request;
	request.setIdToken(cachedSignUp ?
						   cachedSignUp->idToken() :
						   q->idToken());
	const auto reply = api->mailRequestVerify(request);
	lastReply = reply->networkReply();
	reply->onSucceeded(q, [this](int) {
		Q_Q(MailAuthenticator);
		Q_EMIT q->requireCode(requireVerification);
		completeSignIn(true);
	});
	reply->onAllErrors(q, [this](const QString &error, int code, QtRestClient::RestReply::Error errorType) {
		Q_Q(MailAuthenticator);
		FirebaseAuthenticator::logError(error, code, errorType);
		qCCritical(logMailAuth) << "Failed to send verification email!";
		Q_EMIT q->guiError(MailAuthenticator::tr("Failed to send account verification e-mail!"));
		completeSignIn(true);
	}, &FirebaseAuthenticator::translateError);
}

bool MailAuthenticatorPrivate::completeSignIn(bool nonReqOnly)
{
	Q_Q(MailAuthenticator);
	if (cachedSignUp &&
		(!nonReqOnly || !requireVerification)) {
		q->completeSignIn(cachedSignUp->localId(),
						  cachedSignUp->idToken(),
						  cachedSignUp->refreshToken(),
						  cachedExpiresAt,
						  cachedSignUp->email());
		cachedSignUp.reset();
		cachedExpiresAt = {};
		return true;
	} else
		return false;
}
