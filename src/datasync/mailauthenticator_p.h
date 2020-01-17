#ifndef QTDATASYNC_MAILAUTHENTICATOR_P_H
#define QTDATASYNC_MAILAUTHENTICATOR_P_H

#include "mailauthenticator.h"
#include "authenticator_p.h"

#include <QtCore/QPointer>
#include <QtCore/QLoggingCategory>

#include "auth_api.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT MailAuthenticatorPrivate : public IAuthenticatorPrivate
{
	Q_DECLARE_PUBLIC(MailAuthenticator)

public:
	static const QString RememberMailKey;
	static const QString MailKey;

	firebase::auth::ApiClient *api = nullptr;
	QPointer<QNetworkReply> lastReply;

	QCryptographicHash::Algorithm hash = QCryptographicHash::Sha3_512;
	bool rememberMail = true;
	bool requireVerification = true;

	std::optional<firebase::auth::SignInResponse> cachedSignUp;
	QDateTime cachedExpiresAt;

	QString pwHash(const QString &password) const;

	void sendCode();
	bool completeSignIn(bool nonReqOnly);
};

Q_DECLARE_LOGGING_CATEGORY(logMailAuth)

}

#endif // QTDATASYNC_MAILAUTHENTICATOR_P_H
