#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <QtCore/qobject.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT SyncManager : public QObject
{
	Q_OBJECT

public:
	enum SyncState {
		Initializing,
		Downloading,
		Uploading,
		Synchronized,
		Error,
		Disconnected //TODO explicit reconnect ONLY an resync, not on normal sync
	};
	Q_ENUM(SyncState)

	explicit SyncManager(QObject *parent = nullptr);
};

}

#endif // SYNCMANAGER_H
