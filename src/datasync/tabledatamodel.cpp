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
	_logCatStr = "qt.datasync.Statemachine.Table." + _type.toUtf8();

	// statemachine
	connect(stateMachine(), &QScxmlStateMachine::reachedStableState,
			this, &TableDataModel::reachedStableState);
	connect(stateMachine(), &QScxmlStateMachine::log,
			this, &TableDataModel::log);

	_watcher = watcher;
	connect(_watcher, &DatabaseWatcher::triggerSync,
			this, &TableDataModel::triggerUpload);
	connect(_watcher, &DatabaseWatcher::triggerResync,
			this, &TableDataModel::triggerResync);
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
	connect(_connector, &RemoteConnector::liveSyncExpired,
			this, &TableDataModel::liveSyncExpired);

	_transformer = transformer;
	connect(_transformer, &ICloudTransformer::transformDownloadDone,
			this, &TableDataModel::transformDownloadDone);
	connect(_transformer, &ICloudTransformer::transformUploadDone,
			this, &TableDataModel::transformUploadDone);
	connect(_transformer, &ICloudTransformer::transformError,
			this, &TableDataModel::transformError);
}

bool TableDataModel::isRunning() const
{
	return !stateMachine()->isActive(QStringLiteral("Stopped"));
}

Engine::TableState TableDataModel::state() const
{
	// "child" states
	if (stateMachine()->isActive(QStringLiteral("Error")))
		return Engine::TableState::Error;
	else if (stateMachine()->isActive(QStringLiteral("LiveSync")))
		return Engine::TableState::LiveSync;
	else if (stateMachine()->isActive(QStringLiteral("Downloading")))
		return Engine::TableState::Downloading;
	else if (stateMachine()->isActive(QStringLiteral("Uploading")))
		return Engine::TableState::Uploading;
	else if (stateMachine()->isActive(QStringLiteral("Synchronized")))
		return Engine::TableState::Synchronized;
	// "compound" states
	else if (stateMachine()->isActive(QStringLiteral("Stopped")))
		return Engine::TableState::Inactive;
	else if (stateMachine()->isActive(QStringLiteral("DelTable")) ||
			 stateMachine()->isActive(QStringLiteral("Active")))
		return Engine::TableState::Initializing;
	else
		return Engine::TableState::Invalid;
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

void TableDataModel::triggerSync(bool force)
{
	if (force)
		stateMachine()->submitEvent(QStringLiteral("forceSync"));
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
	Q_ASSERT(_connector);
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
	if (!_syncQueue.isEmpty()) {
		const auto data = _syncQueue.dequeue();
		if (_watcher->shouldStore(_transformer->unescapeKey(data.key()), data))
			_transformer->transformDownload(data);
		else
			stateMachine()->submitEvent(QStringLiteral("procContinue"));
	} else {
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
		Q_EMIT errorOccured(_type, error);
	_errors.clear();
}

void TableDataModel::scheduleLsRestart()
{
	_lsRestartTimer->start(seconds{static_cast<qint64>(std::pow(5, _lsErrorCnt - 1))});
}

void TableDataModel::clearLsRestart()
{
	_lsRestartTimer->stop();
}

void TableDataModel::delTable()
{
	Q_ASSERT_X(_passiveSyncToken == RemoteConnector::InvalidToken, Q_FUNC_INFO, "_passiveSyncToken should be invalid");
	_passiveSyncToken = _connector->removeTable(_escType);
}

void TableDataModel::emitStop()
{
	Q_EMIT stopped(_type);
}

void TableDataModel::switchMode()
{
	if (_liveSync)
		stateMachine()->submitEvent(QStringLiteral("startLiveSync"));
	else
		stateMachine()->submitEvent(QStringLiteral("startPassiveSync"));
}

std::optional<QDateTime> TableDataModel::lastSync()
{
	const auto tStamp = _watcher->lastSync(_type);
	if (!tStamp)  // error emitted by watcher
		return std::nullopt;

	if (_cachedLastSync.isValid()) {
		if (tStamp->isValid())
			return std::max(*tStamp, _cachedLastSync);
		else
			return _cachedLastSync;
	} else
		return tStamp;
}

QLoggingCategory TableDataModel::logTableSm() const
{
	return QLoggingCategory{_logCatStr.constData()};
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
	_cachedLastSync = {};
	_syncQueue.clear();
}

void TableDataModel::reachedStableState()
{
	qCDebug(logTableSm) << "Reached state:" << stateMachine()->activeStateNames(true);
	Q_EMIT stateChanged(state());
}

void TableDataModel::log(const QString &label, const QString &msg)
{
	if (label == QStringLiteral("debug"))
		qCDebug(logTableSm).noquote() << msg;
	else if (label == QStringLiteral("info"))
		qCInfo(logTableSm).noquote() << msg;
	else if (label == QStringLiteral("warning"))
		qCWarning(logTableSm).noquote() << msg;
	else if (label == QStringLiteral("critical"))
		qCCritical(logTableSm).noquote() << msg;
	else
		qCDebug(logTableSm).noquote().nospace() << label << ": " << msg;
}

void TableDataModel::triggerUpload(const QString &type)
{
	if (type == _type)
		stateMachine()->submitEvent(QStringLiteral("triggerUpload"));
}

void TableDataModel::triggerResync(const QString &type, bool deleteTable)
{
	if (type == _type) {
		if (deleteTable) {
			_delTable = true;
			stateMachine()->submitEvent(QStringLiteral("delTable"));
		} else {
			stateMachine()->submitEvent(QStringLiteral("forceSync"));
			stateMachine()->submitEvent(QStringLiteral("triggerSync"));
		}
	}
}

void TableDataModel::databaseError(DatabaseWatcher::ErrorScope scope, const QString &message, const QVariant &key, const QSqlError &sqlError)
{
	// filter signal, as multiple statemachines may use the same watcher
	EnginePrivate::ErrorInfo error;
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
	case DatabaseWatcher::ErrorScope::Transaction:
		if ((key.userType() == QMetaType::QString && key.toString() == _type) ||
			(key.userType() == qMetaTypeId<ObjectKey>() && key.value<ObjectKey>().typeName == _type))
			error.type = Engine::ErrorType::Transaction;
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

void TableDataModel::removedTable(const QString &type)
{
	if (type == _escType) {
		_passiveSyncToken = RemoteConnector::InvalidToken;
		stateMachine()->submitEvent(QStringLiteral("delTableDone"));
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
			Q_EMIT errorOccured(_type, {
									Engine::ErrorType::LiveSyncSoft,
									error + tr(" Reconnecting (attempt %n)…", "", _lsErrorCnt),
									type
								});
			stateMachine()->submitEvent(QStringLiteral("lsError"));
		} else {
			setLiveSyncEnabled(false);  // disable livesync for this table
			Q_EMIT errorOccured(_type, {
									Engine::ErrorType::LiveSyncHard,
									error + tr(" Live-synchronization has been disabled!"),
									type
								});
		}
	}
}

void TableDataModel::liveSyncExpired(const QString &type)
{
	// automatically reconnect without user notice
	if (type == _escType) {
		_liveSyncToken = RemoteConnector::InvalidToken;
		_lsErrorCnt = 0;
		stateMachine()->submitEvent(QStringLiteral("lsError"));
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
		_watcher->markCorrupted(key, QDateTime::currentDateTimeUtc());
		_errors.append({
			Engine::ErrorType::Transform,
			message,
			key
		});
		stateMachine()->submitEvent(QStringLiteral("error"));
	}
}
