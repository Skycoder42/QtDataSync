#ifndef QTDATASYNC_DATASTORE_H
#define QTDATASYNC_DATASTORE_H

#include <functional>

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qvariant.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/objectkey.h"
#include "QtDataSync/exception.h"
#include "QtDataSync/qtdatasync_helpertypes.h"

namespace QtDataSync {

class Defaults;

class DataStorePrivate;
//! Main store to generically access all stored data synchronously
class Q_DATASYNC_EXPORT DataStore : public QObject
{
	Q_OBJECT
	friend class DataStoreModel;

public:
	//! Possible pattern modes for the search mechanism
	enum SearchMode
	{
		RegexpMode, //!< Interpret the search string as a regular expression. See QRegularExpression
		WildcardMode, //!< Interpret the search string as a wildcard string (with * and ?)
		ContainsMode, //!< The data key must contain the search string
		StartsWithMode, //!< The data key must start with the search string
		EndsWithMode //!< The data key must end with the search string
	};
	Q_ENUM(SearchMode)

	//! Default constructor, uses the default setup
	explicit DataStore(QObject *parent = nullptr);
	//! Constructor with an explicit setup
	explicit DataStore(const QString &setupName, QObject *parent = nullptr);
	~DataStore();

	//! @copybrief DataStore::count() const
	qint64 count(int metaTypeId) const;
	//! @copybrief DataStore::keys() const
	QStringList keys(int metaTypeId) const;
	//! @copybrief DataStore::loadAll() const
	QVariantList loadAll(int metaTypeId) const;
	//! @copybrief DataStore::load(const QString &) const
	QVariant load(int metaTypeId, const QString &key) const;
	//! @copybrief DataStore::load(int, const QString &) const
	inline QVariant load(int metaTypeId, const QVariant &key) const {
		return load(metaTypeId, key.toString());
	}
	//! @copybrief DataStore::save(const T &)
	void save(int metaTypeId, QVariant value);
	//! @copybrief DataStore::remove(const QString &)
	bool remove(int metaTypeId, const QString &key);
	//! @copybrief DataStore::remove(int, const QString &)
	inline bool remove(int metaTypeId, const QVariant &key) {
		return remove(metaTypeId, key.toString());
	}
	//! @copybrief DataStore::update(T) const
	void update(int metaTypeId, QObject *object) const;
	//! @copybrief DataStore::search(const QString &, SearchMode) const
	QVariantList search(int metaTypeId, const QString &query, SearchMode mode = RegexpMode) const;
	//! @copybrief DataStore::iterate(const std::function<bool(T)> &) const
	void iterate(int metaTypeId,
				 const std::function<bool(QVariant)> &iterator) const;
	//! @copybrief DataStore::clear()
	void clear(int metaTypeId);

	//! Counts the number of datasets for the given type
	template<typename T>
	quint64 count() const;
	//! Returns all saved keys for the given type
	template<typename T>
	QStringList keys() const;
	/*! @copybrief DataStore::keys() const
	 * @tparam K The type of the key to be returned as list
	 * @copydetails DataStore::keys() const
	 * @note The given type K must be convertible from a QString
	 */
	template<typename T, typename K>
	QList<K> keys() const;
	//! Loads all existing datasets for the given type
	template<typename T>
	QList<T> loadAll() const;
	//! Loads the dataset with the given key for the given type
	template<typename T>
	T load(const QString &key) const;
	//! @copybrief DataStore::load(const QString &) const
	template<typename T, typename K>
	T load(const K &key) const;
	//! Saves the given dataset in the store
	template<typename T>
	void save(const T &value);
	//! Removes the dataset with the given key for the given type
	template<typename T>
	bool remove(const QString &key);
	//! @copybrief DataStore::remove(const QString &)
	template<typename T, typename K>
	bool remove(const K &key);
	//! Loads the dataset with the given key for the given type into the existing object by updating it's properties
	template<typename T>
	void update(T object) const;
	//! Searches the store for datasets of the given type where the key matches the query
	template<typename T>
	QList<T> search(const QString &query, SearchMode mode = RegexpMode) const;
	//! Iterates over all existing datasets of the given types
	template<typename T>
	void iterate(const std::function<bool(T)> &iterator) const;
	//! Removes all datasets of the given type from the store
	template<typename T>
	void clear();

Q_SIGNALS:
	//! Is emitted whenever a dataset has been changed
	void dataChanged(int metaTypeId, const QString &key, bool deleted, QPrivateSignal);
	//! Is emitted when a datatypes has been cleared
	void dataCleared(int metaTypeId, QPrivateSignal);
	//! Is emitted when the store is resetted due to an account reset
	void dataResetted(QPrivateSignal);

protected:
	//! @private
	explicit DataStore(QObject *parent, void*);
	//! @private
	void initStore(const QString &setupName);

private:
	QScopedPointer<DataStorePrivate> d;
};



//! Exception that is thrown from DataStore operations in case of an error
class Q_DATASYNC_EXPORT DataStoreException : public Exception
{
protected:
	//! @private
	DataStoreException(const Defaults &defaults, const QString &message);
	//! @private
	DataStoreException(const DataStoreException * const other);
};

//! Exception that is thrown when an internal (critical) error occurs
class Q_DATASYNC_EXPORT LocalStoreException : public DataStoreException
{
public:
	//! @private
	LocalStoreException(const Defaults &defaults, const ObjectKey &key, const QString &context, const QString &message);

	//! The object key of the dataset that triggered the error
	ObjectKey key() const;
	//! The context in which the error occured
	QString context() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	//! @private
	LocalStoreException(const LocalStoreException * const other);

	//! @private
	ObjectKey _key;
	//! @private
	QString _context;
};

//! Exception that is thrown in case no data can be found for a key
class Q_DATASYNC_EXPORT NoDataException : public DataStoreException
{
public:
	//! @private
	NoDataException(const Defaults &defaults, const ObjectKey &key);

	//! The object key of the dataset that does not exist
	ObjectKey key() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	//! @private
	NoDataException(const NoDataException * const other);

	//! @private
	ObjectKey _key;
};

//! Exception that is thrown when unsaveable/unloadable data is passed to the store
class Q_DATASYNC_EXPORT InvalidDataException : public DataStoreException
{
public:
	//! @private
	InvalidDataException(const Defaults &defaults, const QByteArray &typeName, const QString &message);

	//! The name of the type that cannot be saved/loaded
	QByteArray typeName() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	//! @private
	InvalidDataException(const InvalidDataException * const other);

	//! @private
	QByteArray _typeName;
};

// ------------- GENERIC IMPLEMENTATION -------------

template<typename T>
quint64 DataStore::count() const
{
	QTDATASYNC_STORE_ASSERT(T);
	return count(qMetaTypeId<T>());
}

template<typename T>
QStringList DataStore::keys() const
{
	QTDATASYNC_STORE_ASSERT(T);
	return keys(qMetaTypeId<T>());
}

template<typename T, typename K>
QList<K> DataStore::keys() const
{
	QTDATASYNC_STORE_ASSERT(T);
	QList<K> rList;
	for(auto k : keys<T>())
		rList.append(QVariant(k).template value<K>());
	return rList;
}

template<typename T>
QList<T> DataStore::loadAll() const
{
	QTDATASYNC_STORE_ASSERT(T);
	QList<T> rList;
	for(auto v : loadAll(qMetaTypeId<T>()))
		rList.append(v.template value<T>());
	return rList;
}

template<typename T>
T DataStore::load(const QString &key) const
{
	QTDATASYNC_STORE_ASSERT(T);
	return load(qMetaTypeId<T>(), key).template value<T>();
}

template<typename T, typename K>
T DataStore::load(const K &key) const
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
void DataStore::update(T object) const
{
	static_assert(__helpertypes::is_object<T>::value, "loadInto can only be used for pointers to QObject extending classes");
	update(qMetaTypeId<T>(), object);
}

template<typename T>
QList<T> DataStore::search(const QString &query, SearchMode mode) const
{
	QTDATASYNC_STORE_ASSERT(T);
	QList<T> rList;
	for(auto v : search(qMetaTypeId<T>(), query, mode))
		rList.append(v.template value<T>());
	return rList;
}

template<typename T>
void DataStore::iterate(const std::function<bool (T)> &iterator) const
{
	QTDATASYNC_STORE_ASSERT(T);
	iterate(qMetaTypeId<T>(), [iterator](QVariant v) {
		return iterator(v.template value<T>());
	});
}

template<typename T>
void DataStore::clear()
{
	QTDATASYNC_STORE_ASSERT(T);
	clear(qMetaTypeId<T>());
}

}

#endif // QTDATASYNC_DATASTORE_H
