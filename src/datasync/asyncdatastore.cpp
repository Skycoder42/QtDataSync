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
	connect(d->engine, &StorageEngine::notifyChanged,
			this, &AsyncDataStore::dataChanged,
			Qt::QueuedConnection);
}

AsyncDataStore::~AsyncDataStore() {}

GenericTask<int> AsyncDataStore::count(int metaTypeId)
{
	return {this, internalCount(metaTypeId)};
}

GenericTask<QStringList> AsyncDataStore::keys(int metaTypeId)
{
	return {this, internalKeys(metaTypeId)};
}

Task AsyncDataStore::loadAll(int dataMetaTypeId, int listMetaTypeId)
{
	return {this, internalLoadAll(dataMetaTypeId, listMetaTypeId)};
}

Task AsyncDataStore::load(int metaTypeId, const QString &key)
{
	return {this, internalLoad(metaTypeId, key)};
}

Task AsyncDataStore::save(int metaTypeId, const QVariant &value)
{
	return {this, internalSave(metaTypeId, value)};
}

Task AsyncDataStore::remove(int metaTypeId, const QString &key)
{
	return {this, internalRemove(metaTypeId, key)};
}

QFutureInterface<QVariant> AsyncDataStore::internalCount(int metaTypeId)
{
	QFutureInterface<QVariant> interface;
	interface.reportStarted();
	QMetaObject::invokeMethod(d->engine, "beginTask", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, interface),
							  Q_ARG(QThread*, thread()),
							  Q_ARG(QtDataSync::StorageEngine::TaskType, StorageEngine::Count),
							  Q_ARG(int, metaTypeId),
							  Q_ARG(QVariant, {}));
	return interface;
}

QFutureInterface<QVariant> AsyncDataStore::internalKeys(int metaTypeId)
{
	QFutureInterface<QVariant> interface;
	interface.reportStarted();
	QMetaObject::invokeMethod(d->engine, "beginTask", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, interface),
							  Q_ARG(QThread*, thread()),
							  Q_ARG(QtDataSync::StorageEngine::TaskType, StorageEngine::Keys),
							  Q_ARG(int, metaTypeId),
							  Q_ARG(QVariant, {}));
	return interface;
}

QFutureInterface<QVariant> AsyncDataStore::internalLoadAll(int dataMetaTypeId, int listMetaTypeId)
{
	QFutureInterface<QVariant> interface;
	interface.reportStarted();
	QMetaObject::invokeMethod(d->engine, "beginTask", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, interface),
							  Q_ARG(QThread*, thread()),
							  Q_ARG(QtDataSync::StorageEngine::TaskType, StorageEngine::LoadAll),
							  Q_ARG(int, dataMetaTypeId),
							  Q_ARG(QVariant, listMetaTypeId));
	return interface;
}

QFutureInterface<QVariant> AsyncDataStore::internalLoad(int metaTypeId, const QString &key)
{
	QFutureInterface<QVariant> interface;
	interface.reportStarted();
	QMetaObject::invokeMethod(d->engine, "beginTask", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, interface),
							  Q_ARG(QThread*, thread()),
							  Q_ARG(QtDataSync::StorageEngine::TaskType, StorageEngine::Load),
							  Q_ARG(int, metaTypeId),
							  Q_ARG(QVariant, key));
	return interface;
}

QFutureInterface<QVariant> AsyncDataStore::internalSave(int metaTypeId, const QVariant &value)
{
	QFutureInterface<QVariant> interface;
	interface.reportStarted();
	QMetaObject::invokeMethod(d->engine, "beginTask", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, interface),
							  Q_ARG(QThread*, thread()),
							  Q_ARG(QtDataSync::StorageEngine::TaskType, StorageEngine::Save),
							  Q_ARG(int, metaTypeId),
							  Q_ARG(QVariant, value));
	return interface;
}

QFutureInterface<QVariant> AsyncDataStore::internalRemove(int metaTypeId, const QString &key)
{
	QFutureInterface<QVariant> interface;
	interface.reportStarted();
	QMetaObject::invokeMethod(d->engine, "beginTask", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, interface),
							  Q_ARG(QThread*, thread()),
							  Q_ARG(QtDataSync::StorageEngine::TaskType, StorageEngine::Remove),
							  Q_ARG(int, metaTypeId),
							  Q_ARG(QVariant, key));
	return interface;
}
