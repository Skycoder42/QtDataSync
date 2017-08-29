#ifndef QTDATASYNC_ASYNCDATASTORE_H
#define QTDATASYNC_ASYNCDATASTORE_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/task.h"

#include <QtCore/qobject.h>
#include <QtCore/qfuture.h>
#include <QtCore/qmetaobject.h>
#include <functional>

namespace QtDataSync {

class Setup;

class AsyncDataStorePrivate;
//! A class to access data asynchronously
class Q_DATASYNC_EXPORT AsyncDataStore : public QObject
{
	Q_OBJECT

public:
	//! Constructs a store for the default setup
	explicit AsyncDataStore(QObject *parent = nullptr);
	//! Constructs a store for the given setup
	explicit AsyncDataStore(const QString &setupName, QObject *parent = nullptr);
	//! Destructor
	~AsyncDataStore();

	//! @copybrief AsyncDataStore::count()
	GenericTask<int> count(int metaTypeId);
	//! @copybrief AsyncDataStore::keys()
	GenericTask<QStringList> keys(int metaTypeId);
	//! @copybrief AsyncDataStore::loadAll()
	Task loadAll(int dataMetaTypeId, int listMetaTypeId);
	//! @copybrief AsyncDataStore::loadAll()
	Task loadAll(int dataMetaTypeId);
	//! @copybrief AsyncDataStore::load(const QString &)
	Task load(int metaTypeId, const QString &key);
	//! @copybrief AsyncDataStore::load(const K &)
	Task load(int metaTypeId, const QVariant &key);
	//! @copybrief AsyncDataStore::save(const T &)
	Task save(int metaTypeId, const QVariant &value);
	//! @copybrief AsyncDataStore::remove(const QString &)
	Task remove(int metaTypeId, const QString &key);
	//! @copybrief AsyncDataStore::remove(const K &)
	Task remove(int metaTypeId, const QVariant &key);
	//! @copybrief AsyncDataStore::search(const QString &)
	Task search(int dataMetaTypeId, int listMetaTypeId, const QString &query);
	//! @copybrief AsyncDataStore::iterate(const std::function<bool(T)> &, const std::function<void(const QException &)> &)
	void iterate(int metaTypeId,
				 const std::function<bool(QVariant)> &iterator,
				 const std::function<void(const QException &)> &onExcept);

	//! Counts the number of datasets for the given type
	template<typename T>
	GenericTask<int> count();
	//! Returns all saved keys for the given type
	template<typename T>
	GenericTask<QStringList> keys();
	//! Loads all existing datasets for the given type
	template<typename T>
	GenericTask<QList<T>> loadAll();
	//! Loads the dataset with the given key for the given type
	template<typename T>
	GenericTask<T> load(const QString &key);
	//! @copybrief AsyncDataStore::load(const QString &)
	template<typename T, typename K>
	GenericTask<T> load(const K &key);
	//! Saves the given dataset in the store
	template<typename T>
	GenericTask<void> save(const T &value);
	//! Removes the dataset with the given key for the given type
	template<typename T>
	GenericTask<bool> remove(const QString &key);
	//! @copybrief AsyncDataStore::remove(const QString &)
	template<typename T, typename K>
	GenericTask<bool> remove(const K &key);
	//! Searches the store for datasets of the given type where the key matches the query
	template<typename T>
	GenericTask<QList<T>> search(const QString &query);
	//! Asynchronously iterates over all existing datasets of the given types
	template<typename T>
	void iterate(const std::function<bool(T)> &iterator,
				 const std::function<void(const QException &)> &onExcept);

	//! Loads the dataset with the given key for the given type into the existing object by updating it's properties
	template<typename T>
	UpdateTask<T> loadInto(const QString &key, const T &object);
	//! @copybrief AsyncDataStore::loadInto(const QString &, const T &)
	template<typename T, typename K>
	UpdateTask<T> loadInto(const K &key, const T &object);

Q_SIGNALS:
	//! Will be emitted when a dataset in the store has changed
	void dataChanged(int metaTypeId, const QString &key, bool wasDeleted);
	//! Will be emitted when the store has be reset (cleared)
	void dataResetted();

private:
	QScopedPointer<AsyncDataStorePrivate> d;

	QFutureInterface<QVariant> internalCount(int metaTypeId);
	QFutureInterface<QVariant> internalKeys(int metaTypeId);
	QFutureInterface<QVariant> internalLoadAll(int dataMetaTypeId, int listMetaTypeId);
	QFutureInterface<QVariant> internalLoad(int metaTypeId, const QString &key);
	QFutureInterface<QVariant> internalSave(int metaTypeId, const QVariant &value);
	QFutureInterface<QVariant> internalRemove(int metaTypeId, const QString &key);
	QFutureInterface<QVariant> internalSearch(int dataMetaTypeId, int listMetaTypeId, const QString &query);

	void internalIterate(int metaTypeId,
						 const QStringList &keys,
						 int index,
						 QVariant res,
						 const std::function<bool(QVariant)> &iterator,
						 const std::function<void(const QException &)> &onExcept);
};

template<typename T>
GenericTask<int> AsyncDataStore::count()
{
	return internalCount(qMetaTypeId<T>());
}

template<typename T>
GenericTask<QStringList> AsyncDataStore::keys()
{
	return internalKeys(qMetaTypeId<T>());
}

template<typename T>
GenericTask<QList<T>> AsyncDataStore::loadAll()
{
	return internalLoadAll(qMetaTypeId<T>(), qMetaTypeId<QList<T>>());
}

template<typename T>
GenericTask<T> AsyncDataStore::load(const QString &key)
{
	return internalLoad(qMetaTypeId<T>(), key);
}

template<typename T, typename K>
GenericTask<T> AsyncDataStore::load(const K &key)
{
	return internalLoad(qMetaTypeId<T>(), QVariant::fromValue(key).toString());
}

template<typename T>
GenericTask<void> AsyncDataStore::save(const T &value)
{
	return internalSave(qMetaTypeId<T>(), QVariant::fromValue(value));
}

template<typename T>
GenericTask<bool> AsyncDataStore::remove(const QString &key)
{
	return internalRemove(qMetaTypeId<T>(), key);
}

template<typename T, typename K>
GenericTask<bool> AsyncDataStore::remove(const K &key)
{
	return internalRemove(qMetaTypeId<T>(), QVariant::fromValue(key).toString());
}

template<typename T>
GenericTask<QList<T>> AsyncDataStore::search(const QString &query)
{
	return internalSearch(qMetaTypeId<T>(), qMetaTypeId<QList<T>>(), query);
}

template<typename T>
void AsyncDataStore::iterate(const std::function<bool(T)> &iterator, const std::function<void(const QException &)> &onExcept)
{
	return iterate(qMetaTypeId<T>(), [iterator](QVariant variant) {
		return iterator(variant.template value<T>());
	}, onExcept);
}

template<typename T>
UpdateTask<T> AsyncDataStore::loadInto(const QString &key, const T &object)
{
	return {object, internalLoad(qMetaTypeId<T>(), key)};
}

template<typename T, typename K>
UpdateTask<T> AsyncDataStore::loadInto(const K &key, const T &object)
{
	return {object, internalLoad(qMetaTypeId<T>(), QVariant::fromValue(key).toString())};
}

}

#endif // QTDATASYNC_ASYNCDATASTORE_H
