#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <QtCore/qobject.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"

namespace QtDataSync {

class SyncManagerPrivate;
class Q_DATASYNC_EXPORT SyncManager : public QObject
{
	Q_OBJECT

	Q_PROPERTY(bool syncEnabled READ isSyncEnabled WRITE setSyncEnabled NOTIFY syncEnabledChanged)
	Q_PROPERTY(SyncState syncState READ syncState NOTIFY syncStateChanged)
	Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
	enum SyncState {
		Initializing,
		Downloading,
		Uploading,
		Synchronized,
		Error,
		Disconnected
	};
	Q_ENUM(SyncState)

	explicit SyncManager(QObject *parent = nullptr, bool blockingConstruct = false);
	explicit SyncManager(const QString &setupName, QObject *parent = nullptr, bool blockingConstruct = false);
	~SyncManager();

	bool isSyncEnabled() const;
	SyncState syncState() const;
	QString lastError() const;

public Q_SLOTS:
	void setSyncEnabled(bool syncEnabled);

	void synchronize();
	void reconnect();

Q_SIGNALS:
	void syncEnabledChanged(bool syncEnabled);
	void syncStateChanged(QtDataSync::SyncManager::SyncState syncState);
	void lastErrorChanged(const QString &lastError);

private:
	SyncManagerPrivate *d;
};

}

Q_DECLARE_METATYPE(QtDataSync::SyncManager::SyncState)

#endif // SYNCMANAGER_H
