#ifndef QTDATASYNC_GOOGLEAUTHENTICATOR_H
#define QTDATASYNC_GOOGLEAUTHENTICATOR_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/authenticator.h"
#include "QtDataSync/setup.h"

namespace QtDataSync {

class GoogleAuthenticatorPrivate;
class Q_DATASYNC_EXPORT GoogleAuthenticator final : public OAuthAuthenticator
{
	Q_OBJECT

	Q_PROPERTY(bool preferNative READ doesPreferNative WRITE setPreferNative NOTIFY preferNativeChanged)

public:
	explicit GoogleAuthenticator(const QString &clientId,
								 const QString &clientSecret,
								 quint16 callbackPort,
								 QObject *parent = nullptr);

	bool doesPreferNative() const;

public Q_SLOTS:
	void setPreferNative(bool preferNative);

Q_SIGNALS:
	void preferNativeChanged(bool preferNative, QPrivateSignal);

protected:
	void init() final;
	void signIn() final;
	void logOut() final;
	void abortRequest() final;

private:
	Q_DECLARE_PRIVATE(GoogleAuthenticator)

	Q_PRIVATE_SLOT(d_func(), void _q_firebaseSignIn())
	Q_PRIVATE_SLOT(d_func(), void _q_oAuthError(const QString &, const QString &))
};

// ------------- Setup extension -------------

namespace __private {

class Q_DATASYNC_EXPORT GoogleAuthenticatorExtensionPrivate final : public SetupExtensionPrivate
{
public:
	QString clientId;
	QString secret;
	quint16 port = 0;

	void extendFromWebConfig(const QJsonObject &config) final;
	void extendFromGSJsonConfig(const QJsonObject &config) final;
	void extendFromGSPlistConfig(QSettings *config) final;
	void testConfigSatisfied(const QLoggingCategory &logCat) final;
	QObject *createInstance(QObject *parent) final;
};

}

template <typename TSetup>
class SetupAuthenticationExtension<TSetup, GoogleAuthenticator>
{
public:
	inline TSetup &setOAuthClientId(QString clientId) {
		d.clientId = std::move(clientId);
		return *static_cast<TSetup>(this);
	}

	inline TSetup &setOAuthClientSecret(QString secret) {
		d.secret = std::move(secret);
		return *static_cast<TSetup>(this);
	}

	inline TSetup &setOAuthClientCallbackPort(quint16 port) {
		d.port = port;
		return *static_cast<TSetup>(this);
	}

protected:
	inline __private::SetupExtensionPrivate *authenticatorD() {
		return &d;
	}

private:
	__private::GoogleAuthenticatorExtensionPrivate d;
};

}

#endif // QTDATASYNC_GOOGLEAUTHENTICATOR_H
