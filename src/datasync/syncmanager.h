#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <functional>

#include <QtCore/qobject.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"

class QRemoteObjectNode;
class SyncManagerPrivateReplica;

namespace QtDataSync {

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

	explicit SyncManager(QObject *parent = nullptr, int timeout = 0);
	explicit SyncManager(const QString &setupName, QObject *parent = nullptr, int timeout = 0);
	explicit SyncManager(QRemoteObjectNode *node, QObject *parent = nullptr, int timeout = 0);
	~SyncManager();

	bool isSyncEnabled() const;
	SyncState syncState() const;
	QString lastError() const;

public Q_SLOTS:
	void setSyncEnabled(bool syncEnabled);

	void synchronize();
	void reconnect();

	void runOnDownloaded(const std::function<void(SyncState)> &resultFn, bool triggerSync = true);
	void runOnSynchronized(const std::function<void(SyncState)> &resultFn, bool triggerSync = true);

Q_SIGNALS:
	void syncEnabledChanged(bool syncEnabled);
	void syncStateChanged(QtDataSync::SyncManager::SyncState syncState);
	void lastErrorChanged(const QString &lastError);

private:
	SyncManagerPrivateReplica *d;

	void runImp(bool downloadOnly, bool triggerSync, const std::function<void(SyncState)> &resultFn);
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const QtDataSync::SyncManager::SyncState &state);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, QtDataSync::SyncManager::SyncState &state);

}

Q_DECLARE_METATYPE(QtDataSync::SyncManager::SyncState)

#endif // SYNCMANAGER_H
