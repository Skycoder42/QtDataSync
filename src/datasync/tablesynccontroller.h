#ifndef QTDATASYNC_TABLESYNCCONTROLLER_H
#define QTDATASYNC_TABLESYNCCONTROLLER_H

#include "QtDataSync/qtdatasync_global.h"

#include <QtCore/qobject.h>

class QSqlDatabase;

namespace QtDataSync {

class Engine;
class TableDataModel;

class TableSyncControllerPrivate;
class Q_DATASYNC_EXPORT TableSyncController : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString table READ table CONSTANT)
	Q_PROPERTY(bool valid READ isValid NOTIFY validChanged)
	Q_PROPERTY(SyncState syncState READ syncState NOTIFY syncStateChanged)
	Q_PROPERTY(bool liveSyncEnabled READ isLiveSyncEnabled WRITE setLiveSyncEnabled NOTIFY liveSyncEnabledChanged)

public:
	enum class SyncState {
		Disabled = 0,
		Stopped,
		Offline,

		Initializing = 10,
		LiveSync,
		Downloading,
		Uploading,
		Synchronized,

		Error = 20,
		TemporaryError,

		Invalid = -1
	};
	Q_ENUM(SyncState)

	QString table() const;
	bool isValid() const;
	SyncState syncState() const;
	bool isLiveSyncEnabled() const;

	QSqlDatabase database() const;

public Q_SLOTS:
	void start();
	void stop();
	void triggerSync(bool reconnectLiveSync = false);

	void setLiveSyncEnabled(bool liveSyncEnabled);

Q_SIGNALS:
	void validChanged(bool valid, QPrivateSignal = {});
	void syncStateChanged(TableSyncController::SyncState syncState, QPrivateSignal = {});
	void liveSyncEnabledChanged(bool liveSyncEnabled, QPrivateSignal = {});

private:
	friend class Engine;
	Q_DECLARE_PRIVATE(TableSyncController)

	explicit TableSyncController(QString &&table,
								 TableDataModel *model,
								 QObject *parent);
};

}

#endif // QTDATASYNC_TABLESYNCCONTROLLER_H
