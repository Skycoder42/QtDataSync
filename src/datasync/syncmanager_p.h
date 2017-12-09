#ifndef SYNCMANAGER_P_H
#define SYNCMANAGER_P_H

#include "qtdatasync_global.h"
#include "syncmanager.h"
#include "defaults.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT SyncManagerPrivate
{
public:
	static const QString keyEnabled;

	SyncManagerPrivate(const QString &setupName, SyncManager *q_ptr);
	void reconnectEngine();

	Defaults defaults;
	QSettings *settings;
};

}

#endif // SYNCMANAGER_P_H
