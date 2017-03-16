#ifndef SETUP_H
#define SETUP_H

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
class Q_DATASYNC_EXPORT Setup
{
	Q_DISABLE_COPY(Setup)

public:
	static const QString DefaultSetup;

	static void setCleanupTimeout(unsigned long timeout);
	static void removeSetup(const QString &name);

	template <typename T>
	static T *authenticatorForSetup(QObject *parent = nullptr, const QString &name = DefaultSetup);

	Setup();
	~Setup();

	QString localDir() const;
	QJsonSerializer *serializer() const;
	LocalStore *localStore() const;
	StateHolder *stateHolder() const;
	RemoteConnector *remoteConnector() const;
	DataMerger *dataMerger() const;
	QVariant property(const QByteArray &key) const;

	Setup &setLocalDir(QString localDir);
	Setup &setSerializer(QJsonSerializer *serializer);
	Setup &setLocalStore(LocalStore *localStore);
	Setup &setStateHolder(StateHolder *stateHolder);
	Setup &setRemoteConnector(RemoteConnector *remoteConnector);
	Setup &setDataMerger(DataMerger *dataMerger);
	Setup &setProperty(const QByteArray &key, const QVariant &data);

	void create(const QString &name = DefaultSetup);

private:
	QScopedPointer<SetupPrivate> d;

	static Authenticator *loadAuthenticator(QObject *parent, const QString &name);
};

template<typename T>
T *Setup::authenticatorForSetup(QObject *parent, const QString &name)
{
	static_assert(std::is_base_of<Authenticator, T>::value, "T must inherit QtDataSync::Authenticator!");
	return static_cast<T*>(loadAuthenticator(parent, name));
}

}

#endif // SETUP_H
