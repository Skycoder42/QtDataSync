#ifndef REMOTECONNECTOR_H
#define REMOTECONNECTOR_H

#include "QtDataSync/qdatasync_global.h"
#include "QtDataSync/defaults.h"
#include "QtDataSync/stateholder.h"

#include <QtCore/qjsonvalue.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qobject.h>
#include <QtCore/qfutureinterface.h>

namespace QtDataSync {

class Authenticator;

class RemoteConnectorPrivate;
//! The class responsible for data exchange with a remote server
class Q_DATASYNC_EXPORT RemoteConnector : public QObject
{
	Q_OBJECT

public:
	//! Describes the current state of the connector
	enum RemoteState {
		RemoteDisconnected,//!< The remote server is "not available"
		RemoteLoadingState,//!< The remote is available, and the remote state is currently beeing loaded
		RemoteReady//!< The remote state was successfully loaded and the connector is ready for synchronization
	};
	Q_ENUM(RemoteState)

	//! Constructor
	explicit RemoteConnector(QObject *parent = nullptr);
	~RemoteConnector();

	//! Called from the engine to initialize the connector
	virtual void initialize(Defaults *defaults);
	//! Called from the engine to finalize the connector
	virtual void finalize();

	//! Method to create an authenticator for this connector
	virtual Authenticator *createAuthenticator(Defaults *defaults, QObject *parent) = 0;

public Q_SLOTS:
	//! Is called if the remote state should be reloaded
	virtual void reloadRemoteState() = 0;
	//! Is called to start a resync operation
	virtual void requestResync() = 0;

	//! Start a download operation for the given type and key
	virtual void download(const ObjectKey &key, const QByteArray &keyProperty) = 0;
	//! Start an upload operation for the given type, key and data
	virtual void upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) = 0;
	//! Start a download operation for the given type and key
	virtual void remove(const ObjectKey &key, const QByteArray &keyProperty) = 0;
	//! Start a "mark as unchanged" operation for the given type and key
	virtual void markUnchanged(const ObjectKey &key, const QByteArray &keyProperty) = 0;

	//! Is internally used to reset the current user identity @sa RemoteConnector::resetUserData
	void resetUserId(QFutureInterface<QVariant> futureInterface,
					 const QVariant &extraData,
					 bool resetLocalStore);

Q_SIGNALS:
	//! Must be emitted every time the connectors state has changed
	void remoteStateChanged(RemoteConnector::RemoteState state, const StateHolder::ChangeHash &remoteChanges = {});
	//! Can be emitted to notify datasync that a dataset on the remote server changed it's state
	void remoteDataChanged(const ObjectKey &key, StateHolder::ChangeState state);
	//! Should be emitted if the authentication failed with the given credentials
	void authenticationFailed(const QString &reason);
	//! Should be emitted when previous authentication errors have been resolved
	void clearAuthenticationError();

	//! Is emitted when an operation was completed successfully
	void operationDone(const QJsonValue &result = QJsonValue::Undefined);
	//! Is emitted when an operation failed
	void operationFailed(const QString &error);

	//! Is emitted if the local store needs to be resetted
	void performLocalReset(bool clearStore);

protected:
	//! Returns the defaults used by the connector
	Defaults *defaults() const;

	//! Generates or loads an unique device id
	QByteArray getDeviceId() const;
	//! Is called to reset the user data and update them
	virtual void resetUserData(const QVariant &extraData, const QByteArray &oldDeviceId) = 0;

private:
	QScopedPointer<RemoteConnectorPrivate> d;
};

}

#endif // REMOTECONNECTOR_H
