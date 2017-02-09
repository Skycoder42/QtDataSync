#ifndef ASYNCDATASTORE_H
#define ASYNCDATASTORE_H

#include "qtdatasync_global.h"
#include "task.h"
#include <QObject>
#include <QFuture>
#include <QMetaProperty>

namespace QtDataSync {

class Setup;

class AsyncDataStorePrivate;
class QTDATASYNCSHARED_EXPORT AsyncDataStore : public QObject
{
	Q_OBJECT

public:
	explicit AsyncDataStore(QObject *parent = nullptr);
	explicit AsyncDataStore(const QString &setupName, QObject *parent = nullptr);
	~AsyncDataStore();

	GenericTask<int> count(int metaTypeId);
	Task loadAll(int dataMetaTypeId, int listMetaTypeId);
	Task load(int metaTypeId, const QString &key);
	Task save(int metaTypeId, const QVariant &value);
	Task remove(int metaTypeId, const QString &key);
	Task removeAll(int metaTypeId);

	template<typename T>
	GenericTask<int> count();
	template<typename T>
	GenericTask<QList<T>> loadAll();
	template<typename T>
	GenericTask<T> load(const QString &key);
	template<typename T>
	GenericTask<void> save(const T &value);
	template<typename T>
	GenericTask<T> remove(const QString &key);
	template<typename T>
	GenericTask<void> removeAll();

private:
	QScopedPointer<AsyncDataStorePrivate> d;

	QFutureInterface<QVariant> internalCount(int metaTypeId);
	QFutureInterface<QVariant> internalLoadAll(int dataMetaTypeId, int listMetaTypeId);
	QFutureInterface<QVariant> internalLoad(int metaTypeId, const QString &key);
	QFutureInterface<QVariant> internalSave(int metaTypeId, const QVariant &value);
	QFutureInterface<QVariant> internalRemove(int metaTypeId, const QString &key);
	QFutureInterface<QVariant> internalRemoveAll(int metaTypeId);
};

template<typename T>
GenericTask<int> AsyncDataStore::count()
{
	return {this, internalCount(qMetaTypeId<T>())};
}

template<typename T>
GenericTask<QList<T>> AsyncDataStore::loadAll()
{
	return {this, internalLoadAll(qMetaTypeId<T>(), qMetaTypeId<QList<T>>())};
}

template<typename T>
GenericTask<T> AsyncDataStore::load(const QString &key)
{
	return {this, internalLoad(qMetaTypeId<T>(), key)};
}

template<typename T>
GenericTask<void> AsyncDataStore::save(const T &value)
{
	return {this, internalSave(qMetaTypeId<T>(), QVariant::fromValue(value))};
}

template<typename T>
GenericTask<T> AsyncDataStore::remove(const QString &key)
{
	return {this, internalRemove(qMetaTypeId<T>(), key)};
}

template<typename T>
GenericTask<void> AsyncDataStore::removeAll()
{
	return {this, internalRemoveAll(qMetaTypeId<T>())};
}

}

#endif // ASYNCDATASTORE_H
