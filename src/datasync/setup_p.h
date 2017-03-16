#ifndef SETUP_P_H
#define SETUP_P_H

#include "qdatasync_global.h"
#include "datamerger.h"
#include "localstore.h"
#include "remoteconnector.h"
#include "setup.h"
#include "stateholder.h"
#include "storageengine_p.h"

#include <QtCore/QMutex>
#include <QtCore/QThread>

#include <QtJsonSerializer/QJsonSerializer>

namespace QtDataSync {

class Q_DATASYNC_EXPORT SetupPrivate
{
	friend class Setup;

public:
	static StorageEngine *engine(const QString &name = Setup::DefaultSetup);

	static void cleanupHandler();

private:
	static QMutex setupMutex;
	static QHash<QString, QPair<QThread*, StorageEngine*>> engines;
	static unsigned long timeout;

	QString localDir;
	QScopedPointer<QJsonSerializer> serializer;
	QScopedPointer<LocalStore> localStore;
	QScopedPointer<StateHolder> stateHolder;
	QScopedPointer<RemoteConnector> remoteConnector;
	QScopedPointer<DataMerger> dataMerger;
	QHash<QByteArray, QVariant> properties;

	SetupPrivate();
};

}

#endif // SETUP_P_H
