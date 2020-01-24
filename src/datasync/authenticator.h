#ifndef QTDATASYNC_AUTHENTICATOR_H
#define QTDATASYNC_AUTHENTICATOR_H

#include "QtDataSync/qtdatasync_global.h"

#include <QtCore/qobject.h>
#include <QtCore/qhash.h>
#include <QtCore/qsettings.h>

namespace QtRestClient {
class RestClient;
}

namespace QtDataSync {

namespace __private {
template <typename... TAuthenticators>
class AuthenticationSelectorPrivate;
}

class FirebaseAuthenticator;

class IAuthenticatorPrivate;
class Q_DATASYNC_EXPORT IAuthenticator : public QObject
{
	Q_OBJECT

protected:
	struct QProtectedSignal {};

public:
	void init(FirebaseAuthenticator *fbAuth);

	// internal methods! do not call
	virtual void signIn() = 0;
	virtual void logOut() = 0;
	virtual void abortRequest() = 0;

Q_SIGNALS:
	void guiError(const QString &message, QProtectedSignal = {});
	void closeGui(QPrivateSignal = {});

protected:
	IAuthenticator(QObject *parent = nullptr);
	IAuthenticator(IAuthenticatorPrivate &dd, QObject *parent);

	virtual void init();

	QSettings *settings() const;
	QtRestClient::RestClient *client() const;
	QString eMail() const;
	QString idToken() const;

	void completeSignIn(QString localId,
						QString idToken,
						QString refreshToken,
						const QDateTime &expiresAt,
						QString email);
	void failSignIn(const QString &errorMessage);

private:
	Q_DECLARE_PRIVATE(IAuthenticator);
};

class OAuthAuthenticatorPrivate;
class Q_DATASYNC_EXPORT OAuthAuthenticator : public IAuthenticator
{
	Q_OBJECT

public:
	void abortRequest() override;

protected:
	OAuthAuthenticator(QObject *parent = nullptr);
	OAuthAuthenticator(OAuthAuthenticatorPrivate &dd, QObject *parent);

	void init() override;
	void signInWithToken(const QUrl &requestUri,
						 const QString &provider,
						 const QString &token);

private:
	Q_DECLARE_PRIVATE(OAuthAuthenticator);
};

class AuthenticationSelectorBasePrivate;
class Q_DATASYNC_EXPORT AuthenticationSelectorBase : public IAuthenticator
{
	Q_OBJECT

	Q_PROPERTY(bool selectionStored READ isSelectionStored WRITE setSelectionStored NOTIFY selectionStoredChanged)
	Q_PROPERTY(IAuthenticator* selected READ selected NOTIFY selectedChanged)

public:
	Q_INVOKABLE QList<int> selectionTypes() const;
	Q_INVOKABLE IAuthenticator *authenticator(int metaTypeId) const;
	Q_INVOKABLE IAuthenticator *select(int metaTypeId, bool autoSignIn = true);

	bool isSelectionStored() const;
	IAuthenticator* selected() const;

	void init() final;
	void signIn() final;
	void logOut() final;
	void abortRequest() final;

public Q_SLOTS:
	void setSelectionStored(bool selectionStored);

Q_SIGNALS:
	void selectAuthenticator(const QList<int> &metaTypeIds, QPrivateSignal = {});

	void selectionStoredChanged(bool selectionStored, QPrivateSignal = {});
	void selectedChanged(IAuthenticator* selected, QPrivateSignal = {});

protected:
	AuthenticationSelectorBase(QObject *parent);

private:
	template <typename... TAuthenticators>
	friend class __private::AuthenticationSelectorPrivate;
	Q_DECLARE_PRIVATE(AuthenticationSelectorBase);

	void addAuthenticator(int metaTypeId, IAuthenticator *authenticator);
};

template <typename... TAuthenticators>
class AuthenticationSelector final : public AuthenticationSelectorBase
{
	static_assert (std::conjunction_v<std::is_base_of_v<IAuthenticator, TAuthenticators>...>, "All TAuthenticators must extend IAuthenticator");
public:
	inline AuthenticationSelector(QObject *parent) :
		AuthenticationSelectorBase{parent}
	{}
};

}

#endif // QTDATASYNC_AUTHENTICATOR_H
