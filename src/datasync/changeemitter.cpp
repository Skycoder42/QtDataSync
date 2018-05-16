#include "changeemitter_p.h"
using namespace QtDataSync;

ChangeEmitter::ChangeEmitter(const Defaults &defaults, QObject *parent) :
	ChangeEmitterSource(parent),
	_cache(defaults.cacheHandle().value<QSharedPointer<EmitterAdapter::CacheInfo>>())
{}

void ChangeEmitter::triggerChange(QObject *origin, const ObjectKey &key, bool deleted, bool changed)
{
	if(changed)
		emit uploadNeeded();
	emit dataChanged(origin, key, deleted);
	emit remoteDataChanged(key, deleted);
}

void ChangeEmitter::triggerClear(QObject *origin, const QByteArray &typeName, const QStringList &ids)
{
	emit uploadNeeded();
	for(const auto &id : ids) {
		emit dataChanged(origin, {typeName, id}, true);
		emit remoteDataChanged({typeName, id}, true);
	}
}

void ChangeEmitter::triggerReset(QObject *origin)
{
	emit uploadNeeded();
	emit dataResetted(origin);
	emit remoteDataResetted();
}

void ChangeEmitter::triggerUpload()
{
	emit uploadNeeded();
}

void ChangeEmitter::triggerRemoteChange(const ObjectKey &key, bool deleted, bool changed)
{
	if(_cache) {
		auto contains = false;
		//check if cached
		{
			QReadLocker _(&_cache->lock);
			contains = _cache->cache.contains(key);
		}
		//if chached, remove
		if(contains) {
			QWriteLocker _(&_cache->lock);
			_cache->cache.remove(key);
		}
	}
	if(changed)
		emit uploadNeeded();
	emit dataChanged(nullptr, key, deleted);
	emit remoteDataChanged(key, deleted);
}

void ChangeEmitter::triggerRemoteClear(const QByteArray &typeName, const QStringList &ids)
{
	if(_cache) {
		QWriteLocker _(&_cache->lock);
		for(const auto &id : ids)
			_cache->cache.remove({typeName, id});
	}
	emit uploadNeeded();
	for(const auto &id : ids) {
		emit dataChanged(nullptr, {typeName, id}, true);
		emit remoteDataChanged({typeName, id}, true);
	}
}

void ChangeEmitter::triggerRemoteReset()
{
	if(_cache) {
		QWriteLocker _(&_cache->lock);
		_cache->cache.clear();
	}
	emit uploadNeeded();
	emit dataResetted(nullptr);
	emit remoteDataResetted();
}
