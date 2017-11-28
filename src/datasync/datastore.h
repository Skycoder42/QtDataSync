#ifndef DATASTORE_H
#define DATASTORE_H

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qvariant.h>

#include <functional>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/objectkey.h"
#include "QtDataSync/exception.h"
#include "QtDataSync/qtdatasync_helpertypes.h"

namespace QtDataSync {

class Defaults;

class DataStorePrivate;
class Q_DATASYNC_EXPORT DataStore : public QObject
{
	Q_OBJECT

public:
	explicit DataStore(QObject *parent = nullptr);
	explicit DataStore(const QString &setupName, QObject *parent = nullptr);
	~DataStore();

	//! @copybrief DataStore::count()
	qint64 count(int metaTypeId) const;
	//! @copybrief DataStore::keys()
	QStringList keys(int metaTypeId) const;
	//! @copybrief DataStore::loadAll()
	QVariantList loadAll(int metaTypeId) const;
	//! @copybrief DataStore::load(const QString &)
	QVariant load(int metaTypeId, const QString &key) const;
	//! @copybrief DataStore::load(const K &)
	inline QVariant load(int metaTypeId, const QVariant &key) const {
		return load(metaTypeId, key.toString());
	}
	//! @copybrief DataStore::save(const T &)
	void save(int metaTypeId, QVariant value);
	//! @copybrief DataStore::remove(const QString &)
	bool remove(int metaTypeId, const QString &key);
	//! @copybrief DataStore::remove(const K &)
	inline bool remove(int metaTypeId, const QVariant &key) {
		return remove(metaTypeId, key.toString());
	}
	//! @copybrief DataStore::search(const QString &)
	QVariantList search(int metaTypeId, const QString &query) const;
	//! @copybrief DataStore::iterate(const std::function<bool(T)> &, const std::function<void(const QException &)> &)
	void iterate(int metaTypeId,
				 const std::function<bool(QVariant)> &iterator,
				 const std::function<void(const QException &)> &onExcept = {}) const;
	void clear(int metaTypeId);
	void update(int metaTypeId, QObject *object);

	//! Counts the number of datasets for the given type
	template<typename T>
	quint64 count();
	//! Returns all saved keys for the given type
	template<typename T>
	QStringList keys();
	//! @copybrief AsyncDataStore::keys()
	template<typename T, typename K>
	QList<K> keys();
	//! Loads all existing datasets for the given type
	template<typename T>
	QList<T> loadAll();
	//! Loads the dataset with the given key for the given type
	template<typename T>
	T load(const QString &key);
	//! @copybrief AsyncDataStore::load(const QString &)
	template<typename T, typename K>
	T load(const K &key);
	//! Saves the given dataset in the store
	template<typename T>
	void save(const T &value);
	//! Removes the dataset with the given key for the given type
	template<typename T>
	bool remove(const QString &key);
	//! @copybrief AsyncDataStore::remove(const QString &)
	template<typename T, typename K>
	bool remove(const K &key);
	//! Searches the store for datasets of the given type where the key matches the query
	template<typename T>
	QList<T> search(const QString &query);
	//! Asynchronously iterates over all existing datasets of the given types
	template<typename T>
	void iterate(const std::function<bool(T)> &iterator,
				 const std::function<void(const QException &)> &onExcept = {});
	template<typename T>
	void clear();

	//! Loads the dataset with the given key for the given type into the existing object by updating it's properties
	template<typename T>
	void update(T object);

Q_SIGNALS:
	void dataChanged(int metaTypeId, const QString &key, bool deleted);
	void dataCleared(int metaTypeId);
	void dataResetted();

private:
	QScopedPointer<DataStorePrivate> d;
};



class Q_DATASYNC_EXPORT DataStoreException : public Exception
{
protected:
	DataStoreException(const Defaults &defaults, const QString &message);
	DataStoreException(const DataStoreException * const other);
};

class Q_DATASYNC_EXPORT LocalStoreException : public DataStoreException
{
public:
	LocalStoreException(const Defaults &defaults, const ObjectKey &key, const QString &context, const QString &message);

	ObjectKey key() const;
	QString context() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	LocalStoreException(const LocalStoreException * const other);

	const ObjectKey _key;
	const QString _context;
};

class Q_DATASYNC_EXPORT NoDataException : public DataStoreException
{
public:
	NoDataException(const Defaults &defaults, const ObjectKey &key);

	ObjectKey key() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	NoDataException(const NoDataException * const other);

	const ObjectKey _key;
};

class Q_DATASYNC_EXPORT InvalidDataException : public DataStoreException
{
public:
	InvalidDataException(const Defaults &defaults, const QByteArray &typeName, const QString &message);

	QByteArray typeName() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	InvalidDataException(const InvalidDataException * const other);

	const QByteArray _typeName;
};

// ------------- GENERIC IMPLEMENTATION -------------

template<typename T>
quint64 DataStore::count()
{
	QTDATASYNC_STORE_ASSERT(T);
	return count(qMetaTypeId<T>());
}

template<typename T>
QStringList DataStore::keys()
{
	QTDATASYNC_STORE_ASSERT(T);
	return keys(qMetaTypeId<T>());
}

template<typename T, typename K>
QList<K> DataStore::keys()
{
	QTDATASYNC_STORE_ASSERT(T);
	auto kList = keys<T>();
	QList<K> rList;
	foreach(auto k, kList)
		rList.append(QVariant(k).template value<K>());
	return rList;
}

template<typename T>
QList<T> DataStore::loadAll()
{
	QTDATASYNC_STORE_ASSERT(T);
	auto mList = loadAll(qMetaTypeId<T>());
	QList<T> rList;
	foreach(auto v, mList)
		rList.append(v.template value<T>());
	return rList;
}

template<typename T>
T DataStore::load(const QString &key)
{
	QTDATASYNC_STORE_ASSERT(T);
	return load(qMetaTypeId<T>(), key).template value<T>();
}

template<typename T, typename K>
T DataStore::load(const K &key)
{
	QTDATASYNC_STORE_ASSERT(T);
	return load(qMetaTypeId<T>(), QVariant::fromValue(key)).template value<T>();
}

template<typename T>
void DataStore::save(const T &value)
{
	QTDATASYNC_STORE_ASSERT(T);
	save(qMetaTypeId<T>(), QVariant::fromValue(value));
}

template<typename T>
bool DataStore::remove(const QString &key)
{
	QTDATASYNC_STORE_ASSERT(T);
	return remove(qMetaTypeId<T>(), key);
}

template<typename T, typename K>
bool DataStore::remove(const K &key)
{
	QTDATASYNC_STORE_ASSERT(T);
	return remove(qMetaTypeId<T>(), QVariant::fromValue(key));
}

template<typename T>
QList<T> DataStore::search(const QString &query)
{
	QTDATASYNC_STORE_ASSERT(T);
	auto mList = search(qMetaTypeId<T>(), query);
	QList<T> rList;
	foreach(auto v, mList)
		rList.append(v.template value<T>());
	return rList;
}

template<typename T>
void DataStore::iterate(const std::function<bool (T)> &iterator, const std::function<void (const QException &)> &onExcept)
{
	QTDATASYNC_STORE_ASSERT(T);
	iterate(qMetaTypeId<T>(), [iterator](QVariant v) {
		return iterator(v.template value<T>());
	}, onExcept);
}

template<typename T>
void DataStore::clear()
{
	QTDATASYNC_STORE_ASSERT(T);
	clear(qMetaTypeId<T>());
}

template<typename T>
void DataStore::update(T object)
{
	static_assert(__helpertypes::is_storable_obj<T>::value, "loadInto can only be used for pointers to QObject extending classes");
	update(qMetaTypeId<T>(), object);
}

}

#endif // DATASTORE_H
