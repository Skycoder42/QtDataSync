#ifndef QTDATASYNC_ENGINEDATAMODEL_H
#define QTDATASYNC_ENGINEDATAMODEL_H

#include "authenticator.h"
#include "remoteconnector_p.h"
#include "engine_p.h"

#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtCore/QLoggingCategory>

#include <QtScxml/QScxmlCppDataModel>

namespace QtDataSync {

class EngineDataModel : public QScxmlCppDataModel
{
	Q_OBJECT
	Q_SCXML_DATAMODEL

	Q_PROPERTY(Engine::EngineState state READ state NOTIFY stateChanged)

public:
	enum class StopEvent {
		Stop,
		LogOut,
		DeleteAcc
	};
	Q_ENUM(StopEvent)

	explicit EngineDataModel(QObject *parent = nullptr);

	void setupModel(FirebaseAuthenticator *authenticator,
					RemoteConnector *connector);

	bool isSyncActive() const;
	Engine::EngineState state() const;

public Q_SLOTS:
	void start();
	void stop();
	void cancel(const EnginePrivate::ErrorInfo &error);

	void logOut();
	bool deleteAccount();

	void allTablesStopped();

Q_SIGNALS:
	void startTableSync(QPrivateSignal = {});
	void stopTableSync(QPrivateSignal = {});
	void errorOccured(const EnginePrivate::ErrorInfo &info, QPrivateSignal = {});

	void stateChanged(Engine::EngineState state, QPrivateSignal = {});

private /*scripts*/:
	bool hasError() const;

	void signIn();
	void stop(StopEvent event);
	void removeUser();
	void cancelRmUser();
	void emitError();

private Q_SLOTS:
	// engine
	void reachedStableState();
	void log(const QString &label, const QString &msg);
	// authenticator
	void signInSuccessful(const QString &userId, const QString &idToken);
	void signInFailed();
	void accountDeleted(bool success);
	// connector
	void removedUser();
	void networkError(const QString &type, bool temporary);

private:
	QPointer<FirebaseAuthenticator> _authenticator;
	QPointer<RemoteConnector> _connector;

	Engine::EngineState _currentState = Engine::EngineState::Invalid;

	RemoteConnector::CancallationToken _rmUserToken = RemoteConnector::InvalidToken;

	StopEvent _stopEv = StopEvent::Stop;
	QList<EnginePrivate::ErrorInfo> _errors;
};

Q_DECLARE_LOGGING_CATEGORY(logEngineSm)

}

#endif // QTDATASYNC_ENGINEDATAMODEL_H
