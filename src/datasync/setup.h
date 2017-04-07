#ifndef QTDATASYNC_SETUP_H
#define QTDATASYNC_SETUP_H

#include "QtDataSync/qdatasync_global.h"

#include <QtCore/qobject.h>

class QJsonSerializer;

namespace QtDataSync {

class LocalStore;
class StateHolder;
class Authenticator;
class RemoteConnector;
class DataMerger;

class SetupPrivate;
//! The class to setup and create datasync instances
class Q_DATASYNC_EXPORT Setup
{
	Q_DISABLE_COPY(Setup)

public:
	//! The default setup name
	static const QString DefaultSetup;

	//! Sets the maximum timeout for shutting down setups
	static void setCleanupTimeout(unsigned long timeout);
	//! Stops the datasync instance and removes it
	static void removeSetup(const QString &name, bool waitForFinished = false);

	//! Creates a new authenticator for the remote connector of the given setup
	template <typename T>
	static T *authenticatorForSetup(QObject *parent = nullptr, const QString &name = DefaultSetup);

	//! Constructor
	Setup();
	//! Destructor
	~Setup();

	//! Returns the setups local directory
	QString localDir() const;
	//! Returns the setups json serializer
	QJsonSerializer *serializer() const;
	//! Returns the setups local store implementation
	LocalStore *localStore() const;
	//! Returns the setups state holder implementation
	StateHolder *stateHolder() const;
	//! Returns the setups remote connector implementation
	RemoteConnector *remoteConnector() const;
	//! Returns the setups data merger implementation
	DataMerger *dataMerger() const;
	//! Returns the additional property with the given key
	QVariant property(const QByteArray &key) const;

	//! Sets the setups local directory
	Setup &setLocalDir(QString localDir);
	//! Sets the setups json serializer
	Setup &setSerializer(QJsonSerializer *serializer);
	//! Sets the setups local store implementation
	Setup &setLocalStore(LocalStore *localStore);
	//! Sets the setups state holder implementation
	Setup &setStateHolder(StateHolder *stateHolder);
	//! Sets the setups remote connector implementation
	Setup &setRemoteConnector(RemoteConnector *remoteConnector);
	//! Sets the setups data merger implementation
	Setup &setDataMerger(DataMerger *dataMerger);
	//! Sets the additional property with the given key to data
	Setup &setProperty(const QByteArray &key, const QVariant &data);

	//! Creates a datasync instance from this setup with the given name
	void create(const QString &name = DefaultSetup);

private:
	QScopedPointer<SetupPrivate> d;

	static Authenticator *loadAuthenticator(QObject *parent, const QString &name);
};

template<typename T>
T *Setup::authenticatorForSetup(QObject *parent, const QString &name)
{
	static_assert(std::is_base_of<Authenticator, T>::value, "T must inherit QtDataSync::Authenticator!");
	return dynamic_cast<T*>(loadAuthenticator(parent, name));
}

}

#endif // QTDATASYNC_SETUP_H
