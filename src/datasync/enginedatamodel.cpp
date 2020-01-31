#include "enginedatamodel_p.h"
#include <QtScxml/QScxmlStateMachine>
using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logEngineSm, "qt.datasync.Statemachine.Engine")

EngineDataModel::EngineDataModel(QObject *parent) :
	QScxmlCppDataModel{parent}
{}

void EngineDataModel::setupModel(FirebaseAuthenticator *authenticator, RemoteConnector *connector)
{
	Q_ASSERT_X(!stateMachine()->isRunning(), Q_FUNC_INFO, "setupModel must be called before the statemachine is started");

	// machine
	connect(stateMachine(), &QScxmlStateMachine::reachedStableState,
			this, &EngineDataModel::reachedStableState);
	connect(stateMachine(), &QScxmlStateMachine::log,
			this, &EngineDataModel::log);

	_authenticator = authenticator;
	connect(_authenticator, &FirebaseAuthenticator::signInSuccessful,
			this, &EngineDataModel::signInSuccessful);
	connect(_authenticator, &FirebaseAuthenticator::signInFailed,
			this, &EngineDataModel::signInFailed);
	connect(_authenticator, &FirebaseAuthenticator::accountDeleted,
			this, &EngineDataModel::accountDeleted);

	_connector = connector;
	connect(_connector, &RemoteConnector::removedUser,
			this, &EngineDataModel::removedUser);
	connect(_connector, &RemoteConnector::networkError,
			this, &EngineDataModel::networkError);
}

bool EngineDataModel::isSyncActive() const
{
	return stateMachine()->isActive(QStringLiteral("TableSync"));
}

Engine::EngineState EngineDataModel::state() const
{
	const auto mo = QMetaEnum::fromType<Engine::EngineState>();
	const auto states = stateMachine()->activeStateNames(true);
	if (states.isEmpty())
		return Engine::EngineState::Inactive;
	else
		return static_cast<Engine::EngineState>(mo.keyToValue(qUtf8Printable(states.last())));
}

void EngineDataModel::start()
{
	stateMachine()->submitEvent(QStringLiteral("start"));
}

void EngineDataModel::stop()
{
	stop(StopEvent::Stop);
}

void EngineDataModel::cancel(const EnginePrivate::ErrorInfo &error)
{
	_errors.append(error);
	stop(StopEvent::Stop);
}

void EngineDataModel::logOut()
{
	_connector->logOut();
	_authenticator->logOut();
	stop(StopEvent::LogOut);  // only triggeres flow if already logged in
}

bool EngineDataModel::deleteAccount()
{
	if (!stateMachine()->isActive(QStringLiteral("SignedIn"))) {
		qCWarning(logEngineSm) << "Must be signed in to delete the account!";
		return false;
	} else {
		stop(StopEvent::DeleteAcc);
		return true;
	}
}

void EngineDataModel::allTablesStopped()
{
	stateMachine()->submitEvent(QStringLiteral("stopped"));
}

bool EngineDataModel::hasError() const
{
	return !_errors.isEmpty();
}

void EngineDataModel::signIn()
{
	_authenticator->signIn();
}

void EngineDataModel::stop(EngineDataModel::StopEvent event)
{
	_stopEv = event;
	stateMachine()->submitEvent(QStringLiteral("stop"));
}

void EngineDataModel::removeUser()
{
	cancelRmUser();
	_rmUserToken = _connector->removeUser();
}

void EngineDataModel::cancelRmUser()
{
	if (_rmUserToken != RemoteConnector::InvalidToken) {
		_connector->cancel(_rmUserToken);
		_rmUserToken = RemoteConnector::InvalidToken;
	}
}

void EngineDataModel::emitError()
{
	for (const auto &error : _errors)
		Q_EMIT errorOccured(error);
	_errors.clear();
}

void EngineDataModel::reachedStableState()
{
	qCDebug(logEngineSm) << "Reached state:" << stateMachine()->activeStateNames(true);
	Q_EMIT stateChanged(state());
}

void EngineDataModel::log(const QString &label, const QString &msg)
{
	if (label == QStringLiteral("debug"))
		qCDebug(logEngineSm).noquote() << msg;
	else if (label == QStringLiteral("info"))
		qCInfo(logEngineSm).noquote() << msg;
	else if (label == QStringLiteral("warning"))
		qCWarning(logEngineSm).noquote() << msg;
	else if (label == QStringLiteral("critical"))
		qCCritical(logEngineSm).noquote() << msg;
	else
		qCDebug(logEngineSm).noquote().nospace() << label << ": " << msg;
}

void EngineDataModel::signInSuccessful(const QString &userId, const QString &idToken)
{
	if (!_connector->isActive())
		_connector->setUser(userId);
	_connector->setIdToken(idToken);
	stateMachine()->submitEvent(QStringLiteral("signedIn"));
}

void EngineDataModel::signInFailed()
{
	_errors.append({Engine::ErrorType::Network, {}});
	stop(StopEvent::Stop);
}

void EngineDataModel::accountDeleted(bool success)
{
	if (success)
		stateMachine()->submitEvent(QStringLiteral("delAccDone"));
	else {
		_errors.append({Engine::ErrorType::Network, {}});
		stop(StopEvent::Stop);
	}
}

void EngineDataModel::removedUser()
{
	_authenticator->deleteUser();
}

void EngineDataModel::networkError(const QString &type, bool temporary)
{
	if (type.isEmpty()) {  // delete user is empty type
		_errors.append({
			temporary ?
				Engine::ErrorType::Temporary :
				Engine::ErrorType::Network,
			{}
		});
		stop(StopEvent::Stop);
	}
}
