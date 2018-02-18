#include "emitteradapter_p.h"
#include "changeemitter_p.h"
using namespace QtDataSync;

EmitterAdapter::EmitterAdapter(QObject *changeEmitter, QSharedPointer<CacheInfo> cacheInfo, QObject *origin) :
	QObject(origin),
	_isPrimary(changeEmitter->metaObject()->inherits(&ChangeEmitter::staticMetaObject)),
	_emitterBackend(changeEmitter),
	_cache(cacheInfo)
{
	if(_isPrimary) {
		connect(_emitterBackend, SIGNAL(dataChanged(QObject*,QtDataSync::ObjectKey,bool)),
				this, SLOT(dataChangedImpl(QObject*,QtDataSync::ObjectKey,bool)),
				Qt::QueuedConnection);
		connect(_emitterBackend, SIGNAL(dataResetted(QObject*,QByteArray)),
				this, SLOT(dataResettedImpl(QObject*,QByteArray)),
				Qt::QueuedConnection);
	} else {
		connect(_emitterBackend, SIGNAL(remoteDataChanged(QtDataSync::ObjectKey,bool)),
				this, SLOT(remoteDataChangedImpl(QtDataSync::ObjectKey,bool)),
				Qt::QueuedConnection);
		connect(_emitterBackend, SIGNAL(remoteDataResetted(QByteArray)),
				this, SLOT(remoteDataResettedImpl(QByteArray)),
				Qt::QueuedConnection);
	}
}

void EmitterAdapter::triggerChange(const ObjectKey &key, bool deleted, bool changed)
{
	if(_isPrimary) {
		QMetaObject::invokeMethod(_emitterBackend, "triggerChange",
								  Qt::QueuedConnection,
								  Q_ARG(QObject*, parent()),
								  Q_ARG(QtDataSync::ObjectKey, key),
								  Q_ARG(bool, deleted),
								  Q_ARG(bool, changed));
		emit dataChanged(key, deleted);//own change
	} else {
		QMetaObject::invokeMethod(_emitterBackend, "triggerRemoteChange",
								  Qt::QueuedConnection,
								  Q_ARG(QtDataSync::ObjectKey, key),
								  Q_ARG(bool, deleted),
								  Q_ARG(bool, changed));
		//no change signal, because operating in passive setup
	}
}

void EmitterAdapter::triggerClear(const QByteArray &typeName)
{
	if(_isPrimary) {
		QMetaObject::invokeMethod(_emitterBackend, "triggerClear",
								  Qt::QueuedConnection,
								  Q_ARG(QObject*, parent()),
								  Q_ARG(QByteArray, typeName));
		emit dataCleared(typeName);
	} else {
		QMetaObject::invokeMethod(_emitterBackend, "triggerRemoteClear",
								  Qt::QueuedConnection,
								  Q_ARG(QByteArray, typeName));
		//no change signal, because operating in passive setup
	}
}

void EmitterAdapter::triggerReset()
{
	if(_isPrimary) {
		QMetaObject::invokeMethod(_emitterBackend, "triggerReset",
								  Qt::QueuedConnection,
								  Q_ARG(QObject*, parent()));
		emit dataResetted();
	} else {
		QMetaObject::invokeMethod(_emitterBackend, "triggerRemoteReset",
								  Qt::QueuedConnection);
		//no change signal, because operating in passive setup
	}
}

void EmitterAdapter::triggerUpload()
{
	QMetaObject::invokeMethod(_emitterBackend, "triggerUpload",
							  Qt::QueuedConnection);
}

void EmitterAdapter::putCached(const ObjectKey &key, const QJsonObject &data, int costs)
{
	if(!_cache)
		return;

	QWriteLocker _(&_cache->lock);
	_cache->cache.insert(key, new QJsonObject(data), costs);
}

void EmitterAdapter::putCached(const QList<ObjectKey> &keys, const QList<QJsonObject> &data, const QList<int> &costs)
{
	Q_ASSERT(keys.size() == data.size());
	Q_ASSERT(keys.size() == costs.size());
	if(!_cache)
		return;

	QWriteLocker _(&_cache->lock);
	for(auto i = 0; i < keys.size(); i++)
		_cache->cache.insert(keys[i], new QJsonObject(data[i]), costs[i]);
}

bool EmitterAdapter::getCached(const ObjectKey &key, QJsonObject &data)
{
	if(!_cache)
		return false;

	QReadLocker _(&_cache->lock);
	auto json = _cache->cache.object(key);
	if(json) {
		data = *json;
		return true;
	} else
		return false;
}

bool EmitterAdapter::dropCached(const ObjectKey &key)
{
	if(!_cache)
		return false;

	//check if cached
	{
		QReadLocker _(&_cache->lock);
		if(!_cache->cache.contains(key))
			return false;
	}

	QWriteLocker _(&_cache->lock);
	return _cache->cache.remove(key);
}

void EmitterAdapter::dropCached(const QByteArray &typeName)
{
	if(!_cache)
		return;

	QWriteLocker _(&_cache->lock);
	for(auto key : _cache->cache.keys()) {
		if(key.typeName == typeName)
			_cache->cache.remove(key);
	}
}

void EmitterAdapter::dropCached()
{
	if(!_cache)
		return;

	QWriteLocker _(&_cache->lock);
	return _cache->cache.clear();
}

void EmitterAdapter::dataChangedImpl(QObject *origin, const ObjectKey &key, bool deleted)
{
	if(origin == nullptr || origin != parent())
		emit dataChanged(key, deleted);
}

void EmitterAdapter::dataResettedImpl(QObject *origin, const QByteArray &typeName)
{
	if(origin == nullptr || origin != parent()) {
		if(typeName.isEmpty())
			emit dataResetted();
		else
			emit dataCleared(typeName);
	}
}

void EmitterAdapter::remoteDataChangedImpl(const ObjectKey &key, bool deleted)
{
	emit dataChanged(key, deleted);
}

void EmitterAdapter::remoteDataResettedImpl(const QByteArray &typeName)
{
	if(typeName.isEmpty())
		emit dataResetted();
	else
		emit dataCleared(typeName);
}



EmitterAdapter::CacheInfo::CacheInfo(int maxSize) :
	lock(),
	cache(maxSize)
{}
