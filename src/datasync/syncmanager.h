#ifndef QTDATASYNC_SYNCMANAGER_H
#define QTDATASYNC_SYNCMANAGER_H

#include <functional>

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/quuid.h>

#include "QtDataSync/qtdatasync_global.h"

class QRemoteObjectNode;
class QRemoteObjectReplica;
class SyncManagerPrivateReplica;

namespace QtDataSync {

class SyncManagerPrivateHolder;
//! Manages the synchronization process and reports its state
class Q_DATASYNC_EXPORT SyncManager : public QObject
{
	Q_OBJECT

	//! Specifies whether synchronization is currently enabled or disabled
	Q_PROPERTY(bool syncEnabled READ isSyncEnabled WRITE setSyncEnabled NOTIFY syncEnabledChanged)
	//! Holds the current synchronization state
	Q_PROPERTY(SyncState syncState READ syncState NOTIFY syncStateChanged)
	//! Holds the progress of the current sync operation
	Q_PROPERTY(qreal syncProgress READ syncProgress NOTIFY syncProgressChanged)
	//! Holds a description of the last internal error
	Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
	//! The possible states the sync engine can be in
	enum SyncState {
		Initializing, //!< Initializing internal stuff and connecting to a remote
		Downloading, //!< Downloading changes from the remote
		Uploading, //!< Uploading changes to the remote
		Synchronized, //!< All changes have been synchronized. The engine is idle
		Error, //!< An internal error occured. Synchronization is paused until reconnect() is called
		Disconnected //!< The remote is not available or sync has been disabled and the engine thus is disconnected
	};
	Q_ENUM(SyncState)

	//! @copydoc AccountManager::AccountManager(QObject*)
	explicit SyncManager(QObject *parent = nullptr);
	//! @copydoc AccountManager::AccountManager(const QString &, QObject*)
	explicit SyncManager(const QString &setupName, QObject *parent = nullptr);
	//! @copydoc AccountManager::AccountManager(QRemoteObjectNode *, QObject*)
	explicit SyncManager(QRemoteObjectNode *node, QObject *parent = nullptr);
	~SyncManager();

	//! @copybrief AccountManager::replica
	Q_INVOKABLE QRemoteObjectReplica *replica() const;

	//! @readAcFn{syncEnabled}
	bool isSyncEnabled() const;
	//! @readAcFn{syncState}
	SyncState syncState() const;
	//! @readAcFn{syncProgress}
	qreal syncProgress() const;
	//! @readAcFn{lastError}
	QString lastError() const;

	//! Performs an operation once all changes have been downloaded
	void runOnDownloaded(const std::function<void(SyncState)> &resultFn, bool triggerSync = true);
	//! Performs an operation once all changes have been synchronized (both directions)
	void runOnSynchronized(const std::function<void(SyncState)> &resultFn, bool triggerSync = true);

public Q_SLOTS:
	//! @writeAcFn{syncEnabled}
	void setSyncEnabled(bool syncEnabled);

	//! Triggers a synchronization
	void synchronize();
	//! Tries to reconnect to the remote
	void reconnect();

Q_SIGNALS:
	//! @notifyAcFn{syncEnabled}
	void syncEnabledChanged(bool syncEnabled, QPrivateSignal);
	//! @notifyAcFn{syncState}
	void syncStateChanged(QtDataSync::SyncManager::SyncState syncState, QPrivateSignal);
	//! @notifyAcFn{syncProgress}
	void syncProgressChanged(qreal syncProgress, QPrivateSignal);
	//! @notifyAcFn{lastError}
	void lastErrorChanged(const QString &lastError, QPrivateSignal);

protected:
	//! @private
	SyncManager(QObject *parent, void *);
	//! @private
	void initReplica(const QString &setupName);
	//! @private
	void initReplica(QRemoteObjectNode *node);

private Q_SLOTS:
	void onInit();
	void onStateReached(const QUuid &id, SyncState state);

private:
	QScopedPointer<SyncManagerPrivateHolder> d;

	void runImp(bool downloadOnly, bool triggerSync, const std::function<void(SyncState)> &resultFn);
};

//! Stream operator to stream into a QDataStream
Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const SyncManager::SyncState &state);
//! Stream operator to stream out of a QDataStream
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, SyncManager::SyncState &state);

}

Q_DECLARE_METATYPE(QtDataSync::SyncManager::SyncState)
Q_DECLARE_TYPEINFO(QtDataSync::SyncManager::SyncState, Q_PRIMITIVE_TYPE);

#endif // QTDATASYNC_SYNCMANAGER_H
