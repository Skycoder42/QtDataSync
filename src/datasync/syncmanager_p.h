#ifndef SYNCMANAGER_P_H
#define SYNCMANAGER_P_H

#include "qtdatasync_global.h"
#include "syncmanager.h"
#include "defaults.h"

#include "remoteconnector_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT SyncManagerPrivate : public QObject
{
	Q_OBJECT

public:
	SyncManagerPrivate(const QString &setupName, SyncManager *q_ptr, bool blockingConstruct);

	RemoteConnector *remoteConnector() const;

	SyncManager *q;
	Defaults defaults;
	QSettings *settings;

	//chached stuff
	bool cEnabled;
	SyncManager::SyncState cState;
	QString cError;

public Q_SLOTS:
	void updateSyncEnabled(bool syncEnabled);
	void updateSyncState(QtDataSync::SyncManager::SyncState state);
	void updateLastError(const QString &error);
};

}

#endif // SYNCMANAGER_P_H
