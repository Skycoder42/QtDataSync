#include "asyncdatastore.h"
#include "setup.h"
using namespace QtDataSync;

AsyncDataStore::AsyncDataStore(QObject *parent) :
	AsyncDataStore(Setup::setup(), parent)
{}

AsyncDataStore::AsyncDataStore(const QString &setupName, QObject *parent) :
	AsyncDataStore(Setup::setup(setupName), parent)
{}

AsyncDataStore::AsyncDataStore(Setup *setup, QObject *parent) :
	QObject(parent)
{
	Q_ASSERT_X(setup, Q_FUNC_INFO, "AsyncDataStore requires a valid setup!");
}

Task AsyncDataStore::loadAll(int metaTypeId)
{
	return {this, internalLoadAll(metaTypeId)};
}

Task AsyncDataStore::load(int metaTypeId, const QString &key)
{
	return {this, internalLoad(metaTypeId, key)};
}

Task AsyncDataStore::save(int metaTypeId, const QString &key, const QVariant &value)
{
	return {this, internalSave(metaTypeId, key, value)};
}

Task AsyncDataStore::remove(int metaTypeId, const QString &key)
{
	return {this, internalRemove(metaTypeId, key)};
}

Task AsyncDataStore::removeAll(int metaTypeId)
{
	return {this, internalRemoveAll(metaTypeId)};
}

QFutureInterface<QVariant> AsyncDataStore::internalLoadAll(int metaTypeId)
{

}

QFutureInterface<QVariant> AsyncDataStore::internalLoad(int metaTypeId, const QString &key)
{

}

QFutureInterface<QVariant> AsyncDataStore::internalSave(int metaTypeId, const QString &key, const QVariant &value)
{

}

QFutureInterface<QVariant> AsyncDataStore::internalRemove(int metaTypeId, const QString &key)
{

}

QFutureInterface<QVariant> AsyncDataStore::internalRemoveAll(int metaTypeId)
{

}
