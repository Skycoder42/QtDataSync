#ifndef QTDATASYNC_TABLEDATAMODEL_H
#define QTDATASYNC_TABLEDATAMODEL_H

#include "cloudtransformer.h"
#include "tablesynccontroller.h"

#include "databasewatcher_p.h"
#include "remoteconnector_p.h"
#include "engine_p.h"

#include <QtCore/QQueue>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>

#include <QtScxml/QScxmlCppDataModel>

namespace QtDataSync {

class TableDataModel : public QScxmlCppDataModel
{
	Q_OBJECT
	Q_SCXML_DATAMODEL

	Q_PROPERTY(SyncState state READ state NOTIFY stateChanged)
	Q_PROPERTY(bool liveSyncEnabled READ isLiveSyncEnabled WRITE setLiveSyncEnabled NOTIFY liveSyncEnabledChanged)

public:
	using SyncState = TableSyncController::SyncState;
	using ErrorType = Engine::ErrorType;

	explicit TableDataModel(QObject *parent = nullptr);

	void setupModel(QString type,
					DatabaseWatcher *watcher,
					RemoteConnector *connector,
					ICloudTransformer *transformer);

	SyncState state() const;
	bool isLiveSyncEnabled() const;

public Q_SLOTS:
	void start(bool restart = false);
	void stop();
	void exit();
	void triggerSync(bool force);
	void triggerExtUpload();

	void setLiveSyncEnabled(bool liveSyncEnabled);

Q_SIGNALS:
	void exited(const QString &table, QPrivateSignal = {});
	void errorOccured(const QString &table,
					  const EnginePrivate::ErrorInfo &info,
					  QPrivateSignal = {});

	void stateChanged(SyncState state, QPrivateSignal = {});
	void liveSyncEnabledChanged(bool liveSyncEnabled, QPrivateSignal = {});

private /*scripts*/:
	void initSync();
	void initLiveSync();
	void downloadChanges();
	void processDownload();
	void uploadData();
	void emitError();
	void scheduleRestart();
	void clearRestart();
	void delTable();
	void tryExit();

	void switchMode();
	std::optional<QDateTime> lastSync();
	QLoggingCategory logTableSm() const;

	void cancelLiveSync();
	void cancelPassiveSync();
	void cancelUpload();
	void cancelAll();

private Q_SLOTS:
	// machine
	void reachedStableState();
	void log(const QString &label, const QString &msg);
	void finished();
	// watcher
	void triggerUpload(const QString &type);
	void triggerResync(const QString &type, bool deleteTable);
	void databaseError(DatabaseWatcher::ErrorScope scope,
					   const QVariant &key,
					   const QSqlError &sqlError);
	// connector
	void onlineChanged(bool online);
	void downloadedData(const QString &type, const QList<CloudData> &data);
	void syncDone(const QString &type);
	void uploadedData(const ObjectKey &key, const QDateTime &modified);
	void triggerDownload(const QString &type);
	void removedTable(const QString &type);
	void networkError(const QString &type, bool temporary);
	void liveSyncExpired(const QString &type);
	// transformer
	void transformDownloadDone(const LocalData &data);
	void transformUploadDone(const CloudData &data);
	void transformError(const ObjectKey &key);

private:
	QPointer<DatabaseWatcher> _watcher;
	QPointer<RemoteConnector> _connector;
	QPointer<ICloudTransformer> _transformer;

	RemoteConnector::CancallationToken _liveSyncToken = RemoteConnector::InvalidToken;
	RemoteConnector::CancallationToken _passiveSyncToken = RemoteConnector::InvalidToken;
	RemoteConnector::CancallationToken _uploadToken = RemoteConnector::InvalidToken;

	QString _type;
	QString _escType;
	QByteArray _logCatStr = "qt.datasync.Statemachine.Table.<Unknown>";

	QDateTime _cachedLastSync;
	QQueue<CloudData> _syncQueue;

	QList<EnginePrivate::ErrorInfo> _errors;
	quint64 _netErrorCnt = 0;
	QTimer *_restartTimer;

	// statemachine variables
	bool _autoStart = true;
	bool _liveSync = false;
	bool _delTable = false;
	bool _dlReady = true;
	bool _procReady = true;
	bool _exit = false;
};

}

#endif // QTDATASYNC_TABLEDATAMODEL_H
