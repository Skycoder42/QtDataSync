#ifndef QTDATASYNC_MAILAUTHENTICATOR_H
#define QTDATASYNC_MAILAUTHENTICATOR_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/authenticator.h"

#include <QtCore/QCryptographicHash>

namespace QtDataSync {

class MailAuthenticatorPrivate;
class Q_DATASYNC_EXPORT MailAuthenticator : public IAuthenticator
{
	Q_OBJECT

	Q_PROPERTY(QCryptographicHash::Algorithm hash READ hash WRITE setHash NOTIFY hashChanged)
	Q_PROPERTY(bool rememberMail READ rememberMail WRITE setRememberMail NOTIFY rememberMailChanged)
	Q_PROPERTY(bool requireVerification READ requireVerification WRITE setRequireVerification NOTIFY requireVerificationChanged)

public:
	explicit MailAuthenticator(QObject *parent = nullptr);

	QCryptographicHash::Algorithm hash() const;
	bool rememberMail() const;
	bool requireVerification() const;

public Q_SLOTS:
	void signUp(const QString &mail, const QString &password);
	void signIn(const QString &mail, const QString &password);
	void verify(const QString &code);
	void reSendVerify();

	void resetPassword(const QString &mail);
	void confirmReset(const QString &code, const QString &password);

	void setHash(QCryptographicHash::Algorithm hash);
	void setRememberMail(bool rememberMail);
	void setRequireVerification(bool requireVerification);

Q_SIGNALS:
	void showSignInDialog(const QString &rememberedMail, QPrivateSignal = {});
	void requireCode(bool required, QPrivateSignal = {});
	void codeVerified(QPrivateSignal = {});

	void hashChanged(QCryptographicHash::Algorithm hash, QPrivateSignal = {});
	void rememberMailChanged(bool rememberMail, QPrivateSignal = {});
	void requireVerificationChanged(bool requireVerification, QPrivateSignal = {});

protected:
	void init() override;
	void signIn() override;
	void logOut() override;
	void abortRequest() override;

private:
	Q_DECLARE_PRIVATE(MailAuthenticator)
};

}

Q_DECLARE_METATYPE(QtDataSync::MailAuthenticator*)

#endif // QTDATASYNC_MAILAUTHENTICATOR_H
