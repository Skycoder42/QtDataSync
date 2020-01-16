#include "authenticator.h"
#include "authenticator_p.h"
#include <QtCore/QAtomicInteger>
using namespace QtDataSync;
using namespace QtDataSync::__private;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::auth;
using namespace std::chrono;
using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(QtDataSync::logAuth, "qt.datasync.IAuthenticator")
Q_LOGGING_CATEGORY(QtDataSync::logSelector, "qt.datasync.AuthenticationSelector")

void IAuthenticator::init(FirebaseAuthenticator *fbAuth)
{
	Q_D(IAuthenticator);
	d->fbAuth = fbAuth;
	setParent(d->fbAuth);
	init();
}

IAuthenticator::IAuthenticator(QObject *parent) :
	IAuthenticator{*new IAuthenticatorPrivate{}, parent}
{}

IAuthenticator::IAuthenticator(IAuthenticatorPrivate &dd, QObject *parent) :
	QObject{dd, parent}
{}

void IAuthenticator::init() {}

QSettings *IAuthenticator::settings() const
{
	Q_D(const IAuthenticator);
	if (d->fbAuth)
		return d->fbAuth->_settings;
	else
		return nullptr;
}

QtRestClient::RestClient *IAuthenticator::client() const
{
	Q_D(const IAuthenticator);
	if (d->fbAuth)
		return d->fbAuth->_api->restClient();
	else
		return nullptr;
}

void IAuthenticator::completeSignIn(QString localId, QString idToken, QString refreshToken, const QDateTime &expiresAt, QString email)
{
	Q_D(IAuthenticator);
	if (d->fbAuth) {
		d->fbAuth->_localId = std::move(localId);
		d->fbAuth->_idToken = std::move(idToken);
		d->fbAuth->_refreshToken = std::move(refreshToken);
		d->fbAuth->_expiresAt = expiresAt.toUTC();
		d->fbAuth->_email = std::move(email);
		d->fbAuth->storeFbConfig();
		d->fbAuth->startRefreshTimer();
		Q_EMIT d->fbAuth->signInSuccessful(d->fbAuth->_localId, d->fbAuth->_idToken);
		Q_EMIT d->fbAuth->_auth->closeGui();  // emit close of "primary" authenticator
	}
}

void IAuthenticator::failSignIn(const QString &errorMessage)
{
	Q_D(IAuthenticator);
	if (d->fbAuth) {
		Q_EMIT d->fbAuth->signInFailed(errorMessage);
		Q_EMIT d->fbAuth->_auth->closeGui();  // emit close of "primary" authenticator
	} else
		Q_EMIT closeGui();
}



const QString AuthenticationSelectorBasePrivate::SelectionKey = QStringLiteral("auth/selection");

QList<int> AuthenticationSelectorBase::selectionTypes() const
{
	Q_D(const AuthenticationSelectorBase);
	return d->authenticators.keys();
}

IAuthenticator *AuthenticationSelectorBase::authenticator(int metaTypeId) const
{
	Q_D(const AuthenticationSelectorBase);
	return d->authenticators.value(metaTypeId);
}

IAuthenticator *AuthenticationSelectorBase::select(int metaTypeId, bool autoSignIn)
{
	Q_D(AuthenticationSelectorBase);
	d->activeAuth = d->authenticators.value(metaTypeId);
	if (!d->activeAuth) {
		qCWarning(logSetup) << "No such authenticator in the selector:" << QMetaType::typeName(metaTypeId);
		return nullptr;
	}

	if (d->selectionStored)
		settings()->setValue(AuthenticationSelectorBasePrivate::SelectionKey, QByteArray{QMetaType::typeName(metaTypeId)});

	qCDebug(logSelector) << "Using selector of type:" << QMetaType::typeName(metaTypeId);
	Q_EMIT selectedChanged(d->activeAuth);

	if (autoSignIn)
		d->activeAuth->signIn();

	return d->activeAuth;
}

bool AuthenticationSelectorBase::isSelectionStored() const
{
	Q_D(const AuthenticationSelectorBase);
	return d->selectionStored;
}

IAuthenticator *AuthenticationSelectorBase::selected() const
{
	Q_D(const AuthenticationSelectorBase);
	return d->activeAuth;
}

void AuthenticationSelectorBase::init()
{
	Q_D(AuthenticationSelectorBase);
	// load selection if enabled
	if (d->selectionStored &&
		settings()->contains(AuthenticationSelectorBasePrivate::SelectionKey)) {
		const auto selectId = settings()->value(AuthenticationSelectorBasePrivate::SelectionKey).toByteArray();
		qCDebug(logSelector) << "Found stored authenticator selection of type" << selectId;
		d->activeAuth = d->authenticators.value(QMetaType::type(selectId));
		if (d->activeAuth)
			Q_EMIT selectedChanged(d->activeAuth);
		else {
			qCWarning(logSelector) << "Stored authenticator" << selectId
								   << "was not found in the authenticator selection!";
			logOut();
		}
	}
}

void AuthenticationSelectorBase::signIn()
{
	Q_D(AuthenticationSelectorBase);
	if (d->activeAuth)
		d->activeAuth->signIn();
	else
		Q_EMIT selectAuthenticator(d->authenticators.keys());
}

void AuthenticationSelectorBase::logOut()
{
	Q_D(AuthenticationSelectorBase);
	settings()->remove(AuthenticationSelectorBasePrivate::SelectionKey);
	d->activeAuth.clear();
	for (auto auth : qAsConst(d->authenticators))
		auth->logOut();
}

void AuthenticationSelectorBase::abortRequest()
{
	Q_D(AuthenticationSelectorBase);
	for (auto auth : qAsConst(d->authenticators))
		auth->abortRequest();
}

void AuthenticationSelectorBase::setSelectionStored(bool selectionStored)
{
	Q_D(AuthenticationSelectorBase);
	if (selectionStored == d->selectionStored)
		return;

	d->selectionStored = selectionStored;
	Q_EMIT selectionStoredChanged(d->selectionStored);
}

AuthenticationSelectorBase::AuthenticationSelectorBase(QObject *parent) :
	IAuthenticator{*new AuthenticationSelectorBasePrivate{}, parent}
{}

void AuthenticationSelectorBase::addAuthenticator(int metaTypeId, IAuthenticator *authenticator)
{
	Q_D(AuthenticationSelectorBase);
	Q_ASSERT(authenticator);
	if (d->authenticators.contains(metaTypeId)) {
		qCWarning(logSelector) << "Duplicate authenticator type" << QMetaType::typeName(metaTypeId)
							   << "- only the first instance of this type is used!";
		authenticator->deleteLater();
	} else {
		authenticator->setParent(this);
		connect(authenticator, &IAuthenticator::guiError,
				this, &AuthenticationSelectorBase::guiError);
		d->authenticators.insert(metaTypeId, authenticator);
	}
}

#include "moc_authenticator.cpp"
