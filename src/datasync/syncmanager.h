#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <functional>

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/quuid.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"

class QRemoteObjectNode;
class QRemoteObjectReplica;
class SyncManagerPrivateReplica;

namespace QtDataSync {

class SyncManagerPrivateHolder;
class Q_DATASYNC_EXPORT SyncManager : public QObject
{
	Q_OBJECT

	Q_PROPERTY(bool syncEnabled READ isSyncEnabled WRITE setSyncEnabled NOTIFY syncEnabledChanged)
	Q_PROPERTY(SyncState syncState READ syncState NOTIFY syncStateChanged)
	Q_PROPERTY(qreal syncProgress READ syncProgress NOTIFY syncProgressChanged)
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

	explicit SyncManager(QObject *parent = nullptr);
	explicit SyncManager(const QString &setupName, QObject *parent = nullptr);
	explicit SyncManager(QRemoteObjectNode *node, QObject *parent = nullptr);
	~SyncManager();

	Q_INVOKABLE QRemoteObjectReplica *replica() const;

	bool isSyncEnabled() const;
	SyncState syncState() const;
	qreal syncProgress() const;
	QString lastError() const;

	void runOnDownloaded(const std::function<void(SyncState)> &resultFn, bool triggerSync = true);
	void runOnSynchronized(const std::function<void(SyncState)> &resultFn, bool triggerSync = true);

public Q_SLOTS:
	void setSyncEnabled(bool syncEnabled);

	void synchronize();
	void reconnect();

Q_SIGNALS:
	void syncEnabledChanged(bool syncEnabled);
	void syncStateChanged(QtDataSync::SyncManager::SyncState syncState);
	void syncProgressChanged(qreal syncProgress);
	void lastErrorChanged(const QString &lastError);

private Q_SLOTS:
	void onInit();
	void onStateReached(const QUuid &id, SyncState state);

protected:
	SyncManager(QObject *parent, void *);
	void initReplica(const QString &setupName);
	void initReplica(QRemoteObjectNode *node);

private:
	QScopedPointer<SyncManagerPrivateHolder> d;

	void runImp(bool downloadOnly, bool triggerSync, const std::function<void(SyncState)> &resultFn);
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const SyncManager::SyncState &state);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, SyncManager::SyncState &state);

}

Q_DECLARE_METATYPE(QtDataSync::SyncManager::SyncState)

#endif // SYNCMANAGER_H
