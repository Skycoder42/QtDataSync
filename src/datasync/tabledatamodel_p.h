#ifndef QTDATASYNC_TABLEDATAMODEL_H
#define QTDATASYNC_TABLEDATAMODEL_H

#include "engine.h"

#include "cloudtransformer.h"
#include "databasewatcher_p.h"
#include "remoteconnector_p.h"

#include <QtCore/QQueue>
#include <QtCore/QTimer>

#include <QtScxml/QScxmlCppDataModel>

namespace QtDataSync {

class TableDataModel : public QScxmlCppDataModel
{
	Q_OBJECT
	Q_SCXML_DATAMODEL

	Q_PROPERTY(bool liveSyncEnabled READ isLiveSyncEnabled WRITE setLiveSyncEnabled NOTIFY liveSyncEnabledChanged)

public:
	explicit TableDataModel(QObject *parent = nullptr);

	void setupModel(QString type,
					DatabaseWatcher *watcher,
					RemoteConnector *connector,
					ICloudTransformer *transformer);

	bool isLiveSyncEnabled() const;

public Q_SLOTS:
	void start();
	void stop();
	void triggerSync();

	void setLiveSyncEnabled(bool liveSyncEnabled);

Q_SIGNALS:
	void errorOccured(Engine::ErrorType type,
					  const QString &errorMessage,
					  const QVariant &errorData,
					  QPrivateSignal = {});

	void liveSyncEnabledChanged(bool liveSyncEnabled, QPrivateSignal = {});

private /*scripts*/:
	void initSync();
	void initLiveSync();
	void downloadChanges();
	void processDownload();
	void uploadData();
	void emitError();
	void scheduleLsRestart();
	void delTable();

	void switchMode();
	std::optional<QDateTime> lastSync() const;

	void cancelLiveSync(bool resetErrorCount = true);
	void cancelPassiveSync();
	void cancelUpload();
	void cancelAll();

private Q_SLOTS:
	// watcher
	void triggerUpload(const QString &type);
	void triggerResync(const QString &type, bool deleteTable);
	void databaseError(DatabaseWatcher::ErrorScope scope,
					   const QString &message,
					   const QVariant &key,
					   const QSqlError &sqlError);
	// connector
	void downloadedData(const QString &type, const QList<CloudData> &data);
	void syncDone(const QString &type);
	void uploadedData(const ObjectKey &key, const QDateTime &modified);
	void triggerDownload(const QString &type);
	void removedTable(const QString &type);
	void networkError(const QString &error, const QString &type);
	void liveSyncError(const QString &error, const QString &type, bool reconnect);
	// transformer
	void transformDownloadDone(const LocalData &data);
	void transformUploadDone(const CloudData &data);
	void transformError(const ObjectKey &key, const QString &message);

private:
	struct ErrorInfo {
		Engine::ErrorType type = Engine::ErrorType::Unknown;
		QString message;
		QVariant data;
	};

	DatabaseWatcher *_watcher = nullptr; // TODO QPointer
	RemoteConnector *_connector = nullptr; // TODO QPointer
	ICloudTransformer *_transformer = nullptr; // TODO QPointer

	RemoteConnector::CancallationToken _liveSyncToken = RemoteConnector::InvalidToken;
	RemoteConnector::CancallationToken _passiveSyncToken = RemoteConnector::InvalidToken;
	RemoteConnector::CancallationToken _uploadToken = RemoteConnector::InvalidToken;

	QString _type;
	QString _escType;

	QDateTime _cachedLastSync;
	QQueue<CloudData> _syncQueue;

	QList<ErrorInfo> _errors;
	quint64 _lsErrorCnt = 0;
	QTimer *_lsRestartTimer;

	// statemachine variables
	bool _liveSync = false;
	bool _delTable = false;
	bool _dlReady = true;
	bool _procReady = true;
};

}

#endif // QTDATASYNC_TABLEDATAMODEL_H
