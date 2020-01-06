#ifndef QTDATASYNC_TABLEDATAMODEL_H
#define QTDATASYNC_TABLEDATAMODEL_H

#include "cloudtransformer.h"
#include "databasewatcher_p.h"
#include "remoteconnector_p.h"

#include <QtCore/QQueue>

#include <QtScxml/QScxmlCppDataModel>

namespace QtDataSync {

class TableDataModel : public QScxmlCppDataModel
{
	Q_OBJECT
	Q_SCXML_DATAMODEL

public:
	explicit TableDataModel(QObject *parent = nullptr);

	void setup(...);

public Q_SLOTS:
	void triggerDownload();

	void setLiveSyncEnabled(bool liveSyncEnabled);

private /*scripts*/:
	void initSync();
	void initLiveSync();
	void downloadChanges();
	void processDownload();
	void uploadData();

	void switchMode();

private Q_SLOTS:
	// watcher
	void triggerUpload(const QString &type);
	// connector
	void downloadedData(const QList<CloudData> &data);
	void syncDone(const QString &type);
	void uploadedData(const ObjectKey &key, const QDateTime &modified);
	void triggerDownload(const QString &type);
	// transformer
	void transformDownloadDone(const LocalData &data);

private:
	ICloudTransformer *_transformer = nullptr;
	DatabaseWatcher *_watcher = nullptr;
	RemoteConnector *_connector = nullptr;

	QString _type;
	QString _escType;

	QDateTime _cachedLastSync;
	QQueue<CloudData> _syncQueue;

	// statemachine variables
	bool _liveSync = false;
	bool _dlReady = true;
	bool _procReady = true;
};

}

#endif // QTDATASYNC_TABLEDATAMODEL_H
