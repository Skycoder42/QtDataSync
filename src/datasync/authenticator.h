#ifndef QTDATASYNC_AUTHENTICATOR_H
#define QTDATASYNC_AUTHENTICATOR_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/engine.h"

#include <QtCore/qobject.h>


namespace QtDataSync {

class Q_DATASYNC_EXPORT IAuthenticator : public QObject
{
	Q_OBJECT

public:
	IAuthenticator(QObject *parent = nullptr);

public Q_SLOTS:
	virtual void signIn() = 0;

Q_SIGNALS:
	void signInSuccessful(const QString &userId, const QString &idToken);
	void signInFailed();

protected:
	IAuthenticator(QObjectPrivate &dd, QObject *parent = nullptr);
};

class OAuthAuthenticatorPrivate;
class Q_DATASYNC_EXPORT OAuthAuthenticator : public IAuthenticator
{
	Q_OBJECT

	Q_PROPERTY(bool preferNative READ doesPreferNative WRITE setPreferNative NOTIFY preferNativeChanged)

public:
	explicit OAuthAuthenticator(const QString &webApiKey, Engine *engine = nullptr);

	bool doesPreferNative() const;

public Q_SLOTS:
	void signIn() override;
	void deleteAccount();

	void setPreferNative(bool preferNative);

Q_SIGNALS:
	void signInRequested(const QUrl &authUrl);

	void preferNativeChanged(bool preferNative, QPrivateSignal);

private:
	Q_DECLARE_PRIVATE(OAuthAuthenticator)
};

}

#endif // QTDATASYNC_AUTHENTICATOR_H
