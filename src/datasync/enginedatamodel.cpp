#include "enginedatamodel_p.h"
#include <QtScxml/QScxmlStateMachine>
using namespace QtDataSync;

EngineDataModel::EngineDataModel(QObject *parent) :
	QScxmlCppDataModel{parent}
{}

void EngineDataModel::start()
{
	stateMachine()->submitEvent(QStringLiteral("start"));
}

void EngineDataModel::stop()
{
	stateMachine()->submitEvent(QStringLiteral("stop"));
}

bool EngineDataModel::hasError() const
{
	return false;
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

void EngineDataModel::signInSuccessful(const QString &userId, const QString &idToken)
{
	if (!_connector->isActive())
		_connector->setUser(userId);
	_connector->setIdToken(idToken);
	stateMachine()->submitEvent(QStringLiteral("signedIn"));
}

void EngineDataModel::signInFailed(const QString &errorMessage)
{
	_errors.append(errorMessage);
	stop();
}

void EngineDataModel::accountDeleted(bool success)
{
	if (!success)
		_errors.append(tr("Failed to delete user from authentication server!"));
	stop();
}

void EngineDataModel::removedUser()
{

}

void EngineDataModel::networkError(const QString &error, const QString &type)
{

}
