#ifndef SETUP_P_H
#define SETUP_P_H

#include "setup.h"
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

	SetupPrivate();
};

}

#endif // SETUP_P_H
