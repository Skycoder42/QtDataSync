#ifndef QTDATASYNC_AUTHENTICATOR_P_H
#define QTDATASYNC_AUTHENTICATOR_P_H

#include "authenticator.h"
#include "setup_impl.h"

#include <QtCore/QPointer>
#include <QtCore/QLoggingCategory>

#include "firebaseauthenticator_p.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT IAuthenticatorPrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(IAuthenticator)

public:
	QPointer<FirebaseAuthenticator> fbAuth;
};

class Q_DATASYNC_EXPORT AuthenticationSelectorBasePrivate : public IAuthenticatorPrivate
{
	Q_DECLARE_PUBLIC(AuthenticationSelectorBase)

public:
	static const QString SelectionKey;

	QHash<int, IAuthenticator*> authenticators;
	QPointer<IAuthenticator> activeAuth;

	bool selectionStored = true;
};

Q_DECLARE_LOGGING_CATEGORY(logAuth)
Q_DECLARE_LOGGING_CATEGORY(logSelector)

}

#endif // QTDATASYNC_AUTHENTICATOR_P_H
