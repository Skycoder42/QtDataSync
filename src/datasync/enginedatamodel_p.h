#ifndef QTDATASYNC_ENGINEDATAMODEL_H
#define QTDATASYNC_ENGINEDATAMODEL_H

#include "authenticator.h"
#include "remoteconnector_p.h"
#include "engine_p.h"

#include <QtCore/QPointer>
#include <QtCore/QStringList>

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

	void setupModel(IAuthenticator *authenticator,
					RemoteConnector *connector);

	bool isSyncActive() const;

public Q_SLOTS:
	void start();
	void stop();
	void cancel(const EnginePrivate::ErrorInfo &error);

	bool logOut();
	bool deleteAccount();

	void allTablesStopped();

Q_SIGNALS:
	void startTableSync(QPrivateSignal = {});
	void stopTableSync(QPrivateSignal = {});
	void errorOccured(const EnginePrivate::ErrorInfo &info, QPrivateSignal = {});

private /*scripts*/:
	bool hasError() const;

	void stop(StopEvent event);
	void removeUser();
	void cancelRmUser();
	void emitError();

private Q_SLOTS:
	// authenticator
	void signInSuccessful(const QString &userId, const QString &idToken);
	void signInFailed(const QString &errorMessage);
	void accountDeleted(bool success);
	// connector
	void removedUser();
	void networkError(const QString &error, const QString &type);

private:
	QPointer<IAuthenticator> _authenticator;
	QPointer<RemoteConnector> _connector;

	RemoteConnector::CancallationToken _rmUserToken = RemoteConnector::InvalidToken;

	StopEvent _stopEv = StopEvent::Stop;
	QList<EnginePrivate::ErrorInfo> _errors;
};

}

#endif // QTDATASYNC_ENGINEDATAMODEL_H
