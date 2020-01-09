#include "tabledatamodel_p.h"
#include <cmath>
#include <QtScxml/QScxmlStateMachine>
using namespace QtDataSync;
using namespace std::chrono;
using namespace std::chrono_literals;

TableDataModel::TableDataModel(QObject *parent) :
	QScxmlCppDataModel{parent},
	_lsRestartTimer{new QTimer{this}}
{
	_lsRestartTimer->setSingleShot(true);
	_lsRestartTimer->setTimerType(Qt::VeryCoarseTimer);
	connect(_lsRestartTimer, &QTimer::timeout,
			this, [this]() {
				stateMachine()->submitEvent(QStringLiteral("continueLiveSync"));
			});
}

void TableDataModel::setupModel(QString type, DatabaseWatcher *watcher, RemoteConnector *connector, ICloudTransformer *transformer)
{
	Q_ASSERT_X(!stateMachine()->isRunning(), Q_FUNC_INFO, "setupModel must be called before the statemachine is started");
	_type = std::move(type);
	_escType = transformer->escapeType(_type);

	_watcher = watcher;
	connect(_watcher, &DatabaseWatcher::triggerSync,
			this, &TableDataModel::triggerUpload);
	connect(_watcher, &DatabaseWatcher::databaseError,
			this, &TableDataModel::databaseError);

	_connector = connector;
	connect(_connector, &RemoteConnector::downloadedData,
			this, &TableDataModel::downloadedData);
	connect(_connector, &RemoteConnector::syncDone,
			this, &TableDataModel::syncDone);
	connect(_connector, &RemoteConnector::uploadedData,
			this, &TableDataModel::uploadedData);
	connect(_connector, &RemoteConnector::triggerSync,
			this, &TableDataModel::triggerDownload);
	connect(_connector, &RemoteConnector::networkError,
			this, &TableDataModel::networkError);
	connect(_connector, &RemoteConnector::liveSyncError,
			this, &TableDataModel::liveSyncError);

	_transformer = transformer;
	connect(_transformer, &ICloudTransformer::transformDownloadDone,
			this, &TableDataModel::transformDownloadDone);
	connect(_transformer, &ICloudTransformer::transformUploadDone,
			this, &TableDataModel::transformUploadDone);
	connect(_transformer, &ICloudTransformer::transformError,
			this, &TableDataModel::transformError);
}

bool TableDataModel::isLiveSyncEnabled() const
{
	return _liveSync;
}

void TableDataModel::start()
{
	stateMachine()->submitEvent(QStringLiteral("start"));
}

void TableDataModel::stop()
{
	stateMachine()->submitEvent(QStringLiteral("stop"));
}

void TableDataModel::triggerSync()
{
	stateMachine()->submitEvent(QStringLiteral("triggerSync"));
}

void TableDataModel::setLiveSyncEnabled(bool liveSyncEnabled)
{
	if (_liveSync == liveSyncEnabled)
		return;

	_liveSync = liveSyncEnabled;
	Q_EMIT liveSyncEnabledChanged(_liveSync);

	if (!stateMachine()->isActive(QStringLiteral("Init")))
		switchMode();
}

void TableDataModel::initSync()
{
	Q_ASSERT(_transformer);  // TODO make a factory, one instance per statemachine
	Q_ASSERT(_watcher);
	Q_ASSERT(_connector);  // TODO one connector per statemachine? bad for network, maybe? (if 1000+ tables -> 1000+ connections)
	switchMode();
}

void TableDataModel::initLiveSync()
{
	cancelLiveSync(false);
	if (const auto tStamp = lastSync(); tStamp)
		_liveSyncToken = _connector->startLiveSync(_type, *tStamp);
}

void TableDataModel::downloadChanges()
{
	cancelPassiveSync();
	if (const auto tStamp = lastSync(); tStamp)
		_passiveSyncToken = _connector->getChanges(_escType, *tStamp);
}

void TableDataModel::processDownload()
{
	if (!_syncQueue.isEmpty())
		_transformer->transformDownload(_syncQueue.dequeue());
	else {
		_cachedLastSync = {};
		stateMachine()->submitEvent(QStringLiteral("procReady"));  // done with processing downloaded data
	}
}

void TableDataModel::uploadData()
{
	const auto data = _watcher->loadData(_type);
	if (data)
		_transformer->transformUpload(*data);
	else
		stateMachine()->submitEvent(QStringLiteral("syncReady"));  // no more table table -> done
}

void TableDataModel::emitError()
{
	for (const auto &error : qAsConst(_errors))
		Q_EMIT errorOccured(error.type, error.message, error.data);
	_errors.clear();
}

void TableDataModel::scheduleLsRestart()
{
	_lsRestartTimer->start(seconds{static_cast<qint64>(std::pow(5, _lsErrorCnt - 1))});
}

void TableDataModel::switchMode()
{
	if (_liveSync)
		stateMachine()->submitEvent(QStringLiteral("starLiveSync"));
	else
		stateMachine()->submitEvent(QStringLiteral("startPassiveSync"));
}

std::optional<QDateTime> TableDataModel::lastSync() const
{
	const auto tStamp = _watcher->lastSync(_type);
	if (!tStamp)
		return std::nullopt;  // TODO report error?

	if (_cachedLastSync.isValid()) {
		if (tStamp->isValid())
			return std::max(*tStamp, _cachedLastSync);
		else
			return _cachedLastSync;
	} else
		return tStamp;
}

void TableDataModel::cancelLiveSync(bool resetErrorCount)
{
	if (_liveSyncToken != RemoteConnector::InvalidToken) {
		_connector->cancel(_liveSyncToken);
		_liveSyncToken = RemoteConnector::InvalidToken;
	}
	if (resetErrorCount)
		_lsErrorCnt = 0;
}

void TableDataModel::cancelPassiveSync()
{
	if (_passiveSyncToken != RemoteConnector::InvalidToken) {
		_connector->cancel(_passiveSyncToken);
		_passiveSyncToken = RemoteConnector::InvalidToken;
	}
}

void TableDataModel::cancelUpload()
{
	if (_uploadToken != RemoteConnector::InvalidToken) {
		_connector->cancel(_uploadToken);
		_uploadToken = RemoteConnector::InvalidToken;
	}
}

void TableDataModel::cancelAll()
{
	cancelPassiveSync();
	cancelLiveSync();
	cancelUpload();
}

void TableDataModel::triggerUpload(const QString &type)
{
	if (type == _type)
		stateMachine()->submitEvent(QStringLiteral("triggerUpload"));
}

void TableDataModel::databaseError(DatabaseWatcher::ErrorScope scope, const QString &message, const QVariant &key, const QSqlError &sqlError)
{
	// filter signal, as multiple statemachines may use the same watcher
	ErrorInfo error;
	switch (scope) {
	case DatabaseWatcher::ErrorScope::Entry:
		if (key.value<ObjectKey>().typeName == _type)
			error.type = Engine::ErrorType::Entry;
		break;
	case DatabaseWatcher::ErrorScope::Table:
		if (key.toString() == _type)
			error.type = Engine::ErrorType::Table;
		break;
	case DatabaseWatcher::ErrorScope::Database:
		if (const auto keyStr = key.toString();
			keyStr.isEmpty() || keyStr == _type)
			error.type = Engine::ErrorType::Database;
		break;
	case DatabaseWatcher::ErrorScope::System:
		if ((key.userType() == QMetaType::QString && key.toString() == _type) ||
			(key.userType() == qMetaTypeId<ObjectKey>() && key.value<ObjectKey>().typeName == _type))
			error.type = Engine::ErrorType::System;
		break;
	}

	if (error.type != Engine::ErrorType::Unknown) {
		error.message = message;
		error.data = QVariantMap {
			{QStringLiteral("key"), key},
			{QStringLiteral("sqlError"), QVariant::fromValue(sqlError)}
		};
		_errors.append(error);
		stateMachine()->submitEvent(QStringLiteral("error"));
	}
}

void TableDataModel::downloadedData(const QString &type, const QList<CloudData> &data)
{
	if (type == _escType) {
		_syncQueue.append(data);
		const auto ldata = data.last();
		_cachedLastSync = data.last().uploaded();
		stateMachine()->submitEvent(QStringLiteral("dataReady"));  // starts data processing if not already running
	}
}

void TableDataModel::syncDone(const QString &type)
{
	if (type == _escType) {
		_passiveSyncToken = RemoteConnector::InvalidToken;
		_lsErrorCnt = 0;
		stateMachine()->submitEvent(QStringLiteral("dlReady"));
	}
}

void TableDataModel::uploadedData(const ObjectKey &key, const QDateTime &modified)
{
	if (key.typeName == _escType) {
		_uploadToken = RemoteConnector::InvalidToken;
		_watcher->markUnchanged(_transformer->unescapeKey(key), modified);
		stateMachine()->submitEvent(QStringLiteral("ulContinue"));  // upload next data
	}
}

void TableDataModel::triggerDownload(const QString &type)
{
	if (type == _escType) {
		_uploadToken = RemoteConnector::InvalidToken;
		stateMachine()->submitEvent(QStringLiteral("triggerSync"));
	}
}

void TableDataModel::networkError(const QString &error, const QString &type)
{
	if (type == _escType) {
		_errors.append({
			Engine::ErrorType::Network,
			error,
			type
		});
		stateMachine()->submitEvent(QStringLiteral("error"));
	}
}

void TableDataModel::liveSyncError(const QString &error, const QString &type, bool reconnect)
{
	if (type == _escType) {
		_liveSyncToken = RemoteConnector::InvalidToken;
		if (reconnect && _lsErrorCnt++ < 3) {
			Q_EMIT errorOccured(Engine::ErrorType::LiveSyncSoft,
								error + tr(" Reconnecting (attempt %n)…", "", _lsErrorCnt),
								type);
			stateMachine()->submitEvent(QStringLiteral("lsError"));
		} else {
			setLiveSyncEnabled(false);  // disable livesync for this table
			Q_EMIT errorOccured(Engine::ErrorType::LiveSyncHard,
								error + tr(" Live-synchronization has been disabled!"),
								type);
		}
	}
}

void TableDataModel::transformDownloadDone(const LocalData &data)
{
	if (data.key().typeName == _type && stateMachine()->isActive(QStringLiteral("Active"))) {
		_watcher->storeData(data);
		stateMachine()->submitEvent(QStringLiteral("procContinue"));  // process next downloaded data
	}
}

void TableDataModel::transformUploadDone(const CloudData &data)
{
	if (data.key().typeName == _escType && stateMachine()->isActive(QStringLiteral("Active")))
		_uploadToken = _connector->uploadChange(data);
}

void TableDataModel::transformError(const ObjectKey &key, const QString &message)
{
	if (key.typeName == _type) {
		_errors.append({
			Engine::ErrorType::Transform,
			message,
			key
		});
		stateMachine()->submitEvent(QStringLiteral("error"));
	}
}
