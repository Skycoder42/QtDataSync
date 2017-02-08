#include "asyncdatastore.h"
#include "asyncdatastore_p.h"
#include "setup_p.h"
using namespace QtDataSync;

AsyncDataStore::AsyncDataStore(QObject *parent) :
	AsyncDataStore(Setup::DefaultSetup, parent)
{}

AsyncDataStore::AsyncDataStore(const QString &setupName, QObject *parent) :
	QObject(parent),
	d(new AsyncDataStorePrivate())
{
	d->engine = SetupPrivate::engine(setupName);
	Q_ASSERT_X(d->engine, Q_FUNC_INFO, "AsyncDataStore requires a valid setup!");
}

AsyncDataStore::~AsyncDataStore() {}

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
	QFutureInterface<QVariant> interface;
	interface.reportStarted();
	QMetaObject::invokeMethod(d->engine, "beginTask", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, interface),
							  Q_ARG(QtDataSync::StorageEngine::TaskType, StorageEngine::LoadAll),
							  Q_ARG(int, metaTypeId),
							  Q_ARG(QString, {}),
							  Q_ARG(QVariant, {}));
	return interface;
}

QFutureInterface<QVariant> AsyncDataStore::internalLoad(int metaTypeId, const QString &key)
{
	QFutureInterface<QVariant> interface;
	interface.reportStarted();
	QMetaObject::invokeMethod(d->engine, "beginTask", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, interface),
							  Q_ARG(QtDataSync::StorageEngine::TaskType, StorageEngine::Load),
							  Q_ARG(int, metaTypeId),
							  Q_ARG(QString, key),
							  Q_ARG(QVariant, {}));
	return interface;
}

QFutureInterface<QVariant> AsyncDataStore::internalSave(int metaTypeId, const QString &key, const QVariant &value)
{
	QFutureInterface<QVariant> interface;
	interface.reportStarted();
	QMetaObject::invokeMethod(d->engine, "beginTask", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, interface),
							  Q_ARG(QtDataSync::StorageEngine::TaskType, StorageEngine::Save),
							  Q_ARG(int, metaTypeId),
							  Q_ARG(QString, key),
							  Q_ARG(QVariant, value));
	return interface;
}

QFutureInterface<QVariant> AsyncDataStore::internalRemove(int metaTypeId, const QString &key)
{
	QFutureInterface<QVariant> interface;
	interface.reportStarted();
	QMetaObject::invokeMethod(d->engine, "beginTask", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, interface),
							  Q_ARG(QtDataSync::StorageEngine::TaskType, StorageEngine::Remove),
							  Q_ARG(int, metaTypeId),
							  Q_ARG(QString, key),
							  Q_ARG(QVariant, {}));
	return interface;
}

QFutureInterface<QVariant> AsyncDataStore::internalRemoveAll(int metaTypeId)
{
	QFutureInterface<QVariant> interface;
	interface.reportStarted();
	QMetaObject::invokeMethod(d->engine, "beginTask", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, interface),
							  Q_ARG(QtDataSync::StorageEngine::TaskType, StorageEngine::RemoveAll),
							  Q_ARG(int, metaTypeId),
							  Q_ARG(QString, {}),
							  Q_ARG(QVariant, {}));
	return interface;
}
