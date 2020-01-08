#include "tabledatamodel_p.h"
#include <QtScxml/QScxmlStateMachine>
using namespace QtDataSync;

TableDataModel::TableDataModel(QObject *parent) :
	QScxmlCppDataModel{parent}
{}

void TableDataModel::setup(...)
{
	// TODO IMPLEMENT
	Q_UNIMPLEMENTED();
}

void TableDataModel::triggerDownload()
{
	stateMachine()->submitEvent(QStringLiteral("triggerSync"));
}

void TableDataModel::setLiveSyncEnabled(bool liveSyncEnabled)
{
	if (_liveSync == liveSyncEnabled)
		return;

	_liveSync = liveSyncEnabled;
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
	cancelLiveSync();
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

void TableDataModel::cancelLiveSync()
{
	if (_liveSyncToken != RemoteConnector::InvalidToken) {
		_connector->cancel(_liveSyncToken);
		_liveSyncToken = RemoteConnector::InvalidToken;
	}
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
