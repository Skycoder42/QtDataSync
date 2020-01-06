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
	Q_ASSERT(_watcher);
	Q_ASSERT(_connector);
	switchMode();
}

void TableDataModel::initLiveSync()
{
	_connector->startLiveSync();  // TODO start for table
}

void TableDataModel::downloadChanges()
{
	auto tStamp = _watcher->lastSync(_type);
	if (!tStamp)
		;  // TODO report error

	if (_cachedLastSync.isValid()) {
		if (tStamp->isValid())
			tStamp = std::max(*tStamp, _cachedLastSync);
		else
			tStamp = _cachedLastSync;
	}

	_connector->getChanges(_escType, *tStamp);
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

void TableDataModel::triggerUpload(const QString &type)
{
	if (type == _type)
		stateMachine()->submitEvent(QStringLiteral("triggerUpload"));
}

void TableDataModel::downloadedData(const QList<CloudData> &data)
{
	_syncQueue.append(data);
	const auto ldata = data.last();
	_cachedLastSync = data.last().uploaded();
	stateMachine()->submitEvent(QStringLiteral("dataReady"));  // starts data processing if not already running
}

void TableDataModel::syncDone(const QString &type)
{
	if (type == _escType)
		stateMachine()->submitEvent(QStringLiteral("dlReady"));
}

void TableDataModel::uploadedData(const ObjectKey &key, const QDateTime &modified)
{
	_watcher->markUnchanged(_transformer->unescapeKey(key), modified);
	stateMachine()->submitEvent(QStringLiteral("ulContinue"));  // upload next data
}

void TableDataModel::triggerDownload(const QString &type)
{
	if (type == _escType)
		stateMachine()->submitEvent(QStringLiteral("triggerSync"));
}

void TableDataModel::transformDownloadDone(const LocalData &data)
{
	if (stateMachine()->isActive(QStringLiteral("Downloading")) ||
		stateMachine()->isActive(QStringLiteral("LiveSync"))) {
		_watcher->storeData(data);
		stateMachine()->submitEvent(QStringLiteral("procContinue"));  // process next downloaded data
	}
}
