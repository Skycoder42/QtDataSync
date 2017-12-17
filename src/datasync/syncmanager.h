#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <QtCore/qobject.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

class SyncManagerPrivate;
class Q_DATASYNC_EXPORT SyncManager : public QObject
{
	Q_OBJECT

	Q_PROPERTY(bool syncEnabled READ isSyncEnabled WRITE setSyncEnabled NOTIFY syncEnabledChanged)

public:
	enum SyncState {
		Initializing,
		Downloading,
		Uploading,
		Synchronized,
		Error,
		Disconnected //NOTE explicit reconnect ONLY an resync, not on normal sync
	};
	Q_ENUM(SyncState)

	explicit SyncManager(QObject *parent = nullptr);
	explicit SyncManager(const QString &setupName, QObject *parent = nullptr);
	~SyncManager();

	bool isSyncEnabled() const;

public Q_SLOTS:
	void setSyncEnabled(bool syncEnabled);

Q_SIGNALS:
	void syncEnabledChanged(bool syncEnabled);

private:
	QScopedPointer<SyncManagerPrivate> d;
};

}

#endif // SYNCMANAGER_H
