#include "enginedatamodel_p.h"
#include <QtScxml/QScxmlStateMachine>
using namespace QtDataSync;

EngineDataModel::EngineDataModel(QObject *parent) :
	QScxmlCppDataModel{parent}
{}

void EngineDataModel::setupModel(IAuthenticator *authenticator, RemoteConnector *connector)
{
	_authenticator = authenticator;
	connect(_authenticator, &IAuthenticator::signInSuccessful,
			this, &EngineDataModel::signInSuccessful);
	connect(_authenticator, &IAuthenticator::signInFailed,
			this, &EngineDataModel::signInFailed);
	connect(_authenticator, &IAuthenticator::accountDeleted,
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

bool EngineDataModel::logOut()
{
	if (!stateMachine()->isActive(QStringLiteral("SignedIn")))
		return false;
	else {
		stop(StopEvent::LogOut);
		return true;
	}
}

bool EngineDataModel::deleteAccount()
{
	if (!stateMachine()->isActive(QStringLiteral("SignedIn")))
		return false;
	else {
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

void EngineDataModel::signInSuccessful(const QString &userId, const QString &idToken)
{
	if (!_connector->isActive())
		_connector->setUser(userId);
	_connector->setIdToken(idToken);
	stateMachine()->submitEvent(QStringLiteral("signedIn"));
}

void EngineDataModel::signInFailed(const QString &errorMessage)
{
	_errors.append({Engine::ErrorType::Network, errorMessage, {}});
	stop(StopEvent::Stop);
}

void EngineDataModel::accountDeleted(bool success)
{
	if (!success)
		_errors.append({Engine::ErrorType::Network, tr("Failed to delete user from authentication server!"), {}});
	stop(StopEvent::Stop);
}

void EngineDataModel::removedUser()
{
	_authenticator->deleteUser();
}

void EngineDataModel::networkError(const QString &error, const QString &type)
{
	if (type.isEmpty()) {
		_errors.append({Engine::ErrorType::Network, error, {}});
		stop(StopEvent::Stop);
	}
}
