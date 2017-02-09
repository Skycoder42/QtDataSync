#ifndef SETUP_P_H
#define SETUP_P_H

#include "datamerger.h"
#include "localstore.h"
#include "remoteconnector.h"
#include "setup.h"
#include "stateholder.h"
#include "storageengine.h"
#include <QJsonSerializer>
#include <QMutex>
#include <QThread>

namespace QtDataSync {

class SetupPrivate
{
	friend class Setup;

public:
	static StorageEngine *engine(const QString &name = Setup::DefaultSetup);

	static void cleanupHandler();

private:
	static QMutex setupMutex;
	static QHash<QString, QPair<QThread*, StorageEngine*>> engines;
	static unsigned long timeout;

	QScopedPointer<QJsonSerializer> serializer;
	QScopedPointer<LocalStore> localStore;
	QScopedPointer<StateHolder> stateHolder;
	QScopedPointer<RemoteConnector> remoteConnector;
	QScopedPointer<DataMerger> dataMerger;

	SetupPrivate();
};

}

#endif // SETUP_P_H
