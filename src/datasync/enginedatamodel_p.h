#ifndef QTDATASYNC_ENGINEDATAMODEL_H
#define QTDATASYNC_ENGINEDATAMODEL_H

#include "authenticator.h"
#include "remoteconnector_p.h"

#include <QtScxml/QScxmlCppDataModel>

namespace QtDataSync {

class EngineDataModel : public QScxmlCppDataModel
{
	Q_OBJECT
	Q_SCXML_DATAMODEL

public:
	enum class StopEvent {
		Stop,
		LogOut,
		DeleteAcc
	};
	Q_ENUM(StopEvent)

	explicit EngineDataModel(QObject *parent = nullptr);

public Q_SLOTS:
	void start();
	void stop();

private /*scripts*/:
	bool hasError() const;

	void removeUser();
	void cancelRmUser();

private Q_SLOTS:
	// authenticator
	void signInSuccessful(const QString &userId, const QString &idToken);
	void signInFailed(const QString &errorMessage);
	void accountDeleted(bool success);
	// connector
	void removedUser();
	void networkError(const QString &error, const QString &type);

private:
	IAuthenticator *_authenticator = nullptr; // TODO QPointer
	RemoteConnector *_connector = nullptr; // TODO QPointer

	RemoteConnector::CancallationToken _rmUserToken = RemoteConnector::InvalidToken;

	StopEvent _stopEv = StopEvent::Stop;
	QStringList _errors;
};

}

#endif // QTDATASYNC_ENGINEDATAMODEL_H
