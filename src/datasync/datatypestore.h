#ifndef QTDATASYNC_DATATYPESTORE_H
#define QTDATASYNC_DATATYPESTORE_H

#include <type_traits>

#include <QtCore/qobject.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/datastore.h"

namespace QtDataSync {

//! Base class for all DataTypeStore like classes
class Q_DATASYNC_EXPORT DataTypeStoreBase : public QObject
{
	Q_OBJECT

public:
	//! Default constructor
	explicit DataTypeStoreBase(QObject *parent = nullptr);

	//! Returns a reference to the internally used DataStore
	virtual DataStore *store() const = 0;

Q_SIGNALS:
	//! Will be emitted when a dataset in the store has changed
	void dataChanged(const QString &key, const QVariant &value);
	//! Will be emitted when the store was reset or cleared
	void dataResetted();
};

//! A wrapper around the DataStore limited to one type to make access simpler
template <typename TType, typename TKey = QString>
class DataTypeStore : public DataTypeStoreBase
{
	QTDATASYNC_STORE_ASSERT(TType);

public:
	//! Constructs a store for the default setup
	explicit DataTypeStore(QObject *parent = nullptr);
	//! Constructs a store for the given setup
	explicit DataTypeStore(const QString &setupName, QObject *parent = nullptr);
	//! Constructs a store for the given setup
	explicit DataTypeStore(DataStore *store, QObject *parent = nullptr);

	DataStore *store() const override;

	//! @copybrief DataStore::count() const
	qint64 count() const;
	//! @copybrief DataStore::keys() const
	QList<TKey> keys() const;
	//! @copybrief DataStore::loadAll() const
	QList<TType> loadAll() const;
	//! @copybrief DataStore::load(const K &) const
	TType load(const TKey &key) const;
	//! @copybrief DataStore::save(const T &)
	void save(const TType &value);
	//! @copybrief DataStore::remove(const K &)
	bool remove(const TKey &key);
	//! @copybrief DataStore::update(T) const
	template <typename TX = TType>
	void update(std::enable_if_t<__helpertypes::is_object<TX>::value, TX> object) const;
	//! @copybrief DataStore::search(const QString &, SearchMode) const
	QList<TType> search(const QString &query, DataStore::SearchMode mode = DataStore::RegexpMode);
	//! @copybrief DataStore::iterate(const std::function<bool(T)> &) const
	void iterate(const std::function<bool(TType)> &iterator);
	//! @copybrief DataStore::clear()
	void clear();

	//! Shortcut to convert a string to the stores key type
	static TKey toKey(const QString &key);

private:
	DataStore *_store;

	void evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void evalDataCleared(int metaTypeId);
};

//! A DataTypeStore that caches all loaded data internally for faster access
template <typename TType, typename TKey = QString>
class CachingDataTypeStore : public DataTypeStoreBase
{
	static_assert(__helpertypes::is_gadget<TType>::value, "TType must be a Q_GADGET");

public:
	//! Typedef for QHash::const_iterator
	typedef typename QHash<TKey, TType>::const_iterator const_iterator;
	//! Typedef for QHash::const_iterator
	typedef const_iterator iterator;

	//! @copydoc DataTypeStore::DataTypeStore(QObject*)
	explicit CachingDataTypeStore(QObject *parent = nullptr);
	//! @copydoc DataTypeStore::DataTypeStore(const QString &, QObject*)
	explicit CachingDataTypeStore(const QString &setupName, QObject *parent = nullptr);
	//! @copydoc DataTypeStore::DataTypeStore(DataStore *, QObject*)
	explicit CachingDataTypeStore(DataStore *store, QObject *parent = nullptr);

	DataStore *store() const override;

	//! @copybrief DataTypeStore::count
	qint64 count() const;
	//! @copybrief DataTypeStore::keys
	QList<TKey> keys() const;
	//! Checks if a dataset for the given key exists
	bool contains(const TKey &key) const;
	//! @copybrief DataTypeStore::loadAll
	QList<TType> loadAll() const;
	//! @copybrief DataTypeStore::load
	TType load(const TKey &key) const;
	//! @copydoc DataTypeStore::save
	void save(const TType &value);
	//! @copydoc DataTypeStore::remove
	bool remove(const TKey &key);
	//! Returns the dataset for the given key and removes it from the store
	TType take(const TKey &key);
	//! @copydoc DataTypeStore::clear
	void clear();

	//! Returns the begin iterator of the internal hash
	const_iterator begin() const;
	//! Returns the end iterator of the internal hash
	const_iterator end() const;

	//! @copydoc DataTypeStore::toKey
	static TKey toKey(const QString &key);

private:
	DataStore *_store;
	QHash<TKey, TType> _data;

	void evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void evalDataCleared(int metaTypeId);
	void evalDataResetted();
};

//! @copydoc QtDataSync::CachingDataTypeStore
template <typename TType, typename TKey>
class CachingDataTypeStore<TType*, TKey> : public DataTypeStoreBase
{
	static_assert(__helpertypes::is_object<TType*>::value, "TType must inherit QObject");

public:
	//! @copybrief CachingDataTypeStore::const_iterator
	typedef typename QHash<TKey, TType*>::const_iterator const_iterator;
	//! @copybrief CachingDataTypeStore::iterator
	typedef const_iterator iterator;

	//!@copydoc CachingDataTypeStore::CachingDataTypeStore(QObject *)
	explicit CachingDataTypeStore(QObject *parent = nullptr);
	//!@copydoc CachingDataTypeStore::CachingDataTypeStore(const QString &, QObject *)
	explicit CachingDataTypeStore(const QString &setupName, QObject *parent = nullptr);
	//!@copydoc CachingDataTypeStore::CachingDataTypeStore(DataStore*, QObject *)
	explicit CachingDataTypeStore(DataStore *store, QObject *parent = nullptr);

	DataStore *store() const override;

	//!@copydoc CachingDataTypeStore::count
	qint64 count() const;
	//!@copydoc CachingDataTypeStore::keys
	QList<TKey> keys() const;
	//!@copydoc CachingDataTypeStore::contains
	bool contains(const TKey &key) const;
	//!@copydoc CachingDataTypeStore::loadAll
	QList<TType*> loadAll() const;
	//!@copydoc CachingDataTypeStore::load
	TType* load(const TKey &key) const;
	//!@copydoc CachingDataTypeStore::save
	void save(TType *value);
	//!@copydoc CachingDataTypeStore::remove
	bool remove(const TKey &key);
	//!@copydoc CachingDataTypeStore::take
	TType* take(const TKey &key);
	//!@copydoc CachingDataTypeStore::clear
	void clear();

	//!@copydoc CachingDataTypeStore::begin
	const_iterator begin() const;
	//!@copydoc CachingDataTypeStore::end
	const_iterator end() const;

	//!@copydoc CachingDataTypeStore::toKey
	static TKey toKey(const QString &key);

private:
	DataStore *_store;
	QHash<TKey, TType*> _data;

	void evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void evalDataCleared(int metaTypeId);
	void evalDataResetted();
};

// ------------- GENERIC IMPLEMENTATION DataTypeStore -------------

template <typename TType, typename TKey>
DataTypeStore<TType, TKey>::DataTypeStore(QObject *parent) :
	DataTypeStore(DefaultSetup, parent)
{}

template <typename TType, typename TKey>
DataTypeStore<TType, TKey>::DataTypeStore(const QString &setupName, QObject *parent) :
	DataTypeStore(new DataStore(setupName, nullptr), parent)
{
	_store->setParent(this);
}

template <typename TType, typename TKey>
DataTypeStore<TType, TKey>::DataTypeStore(DataStore *store, QObject *parent) :
	DataTypeStoreBase(parent),
	_store(store)
{
	connect(_store, &DataStore::dataChanged,
			this, &DataTypeStore::evalDataChanged);
	connect(_store, &DataStore::dataCleared,
			this, &DataTypeStore::evalDataCleared);
	connect(_store, &DataStore::dataResetted,
			this, &DataTypeStore::dataResetted);
}

template<typename TType, typename TKey>
DataStore *DataTypeStore<TType, TKey>::store() const
{
	return _store;
}

template <typename TType, typename TKey>
qint64 DataTypeStore<TType, TKey>::count() const
{
	return _store->count<TType>();
}

template <typename TType, typename TKey>
QList<TKey> DataTypeStore<TType, TKey>::keys() const
{
	return _store->keys<TType, TKey>();
}

template <typename TType, typename TKey>
QList<TType> DataTypeStore<TType, TKey>::loadAll() const
{
	return _store->loadAll<TType>();
}

template <typename TType, typename TKey>
TType DataTypeStore<TType, TKey>::load(const TKey &key) const
{
	return _store->load<TType>(key);
}

template <typename TType, typename TKey>
void DataTypeStore<TType, TKey>::save(const TType &value)
{
	_store->save(value);
}

template <typename TType, typename TKey>
bool DataTypeStore<TType, TKey>::remove(const TKey &key)
{
	return _store->remove<TType>(key);
}

template<typename TType, typename TKey>
template <typename TX>
void DataTypeStore<TType, TKey>::update(std::enable_if_t<__helpertypes::is_object<TX>::value, TX> object) const
{
	_store->update<TType>(object);
}

template<typename TType, typename TKey>
QList<TType> DataTypeStore<TType, TKey>::search(const QString &query, DataStore::SearchMode mode)
{
	return _store->search<TType>(query, mode);
}

template<typename TType, typename TKey>
void DataTypeStore<TType, TKey>::iterate(const std::function<bool (TType)> &iterator)
{
	_store->iterate(iterator);
}

template<typename TType, typename TKey>
void DataTypeStore<TType, TKey>::clear()
{
	_store->clear<TType>();
}

template<typename TType, typename TKey>
TKey DataTypeStore<TType, TKey>::toKey(const QString &key)
{
	return QVariant(key).value<TKey>();
}

template <typename TType, typename TKey>
void DataTypeStore<TType, TKey>::evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted)
{
	if(metaTypeId == qMetaTypeId<TType>()) {
		if(wasDeleted)
			emit dataChanged(key, QVariant());
		else
			emit dataChanged(key, QVariant::fromValue(_store->load<TType>(key)));
	}
}

template<typename TType, typename TKey>
void DataTypeStore<TType, TKey>::evalDataCleared(int metaTypeId)
{
	if(metaTypeId == qMetaTypeId<TType>())
		emit dataResetted();
}

// ------------- GENERIC IMPLEMENTATION CachingDataTypeStore -------------

template <typename TType, typename TKey>
CachingDataTypeStore<TType, TKey>::CachingDataTypeStore(QObject *parent) :
	CachingDataTypeStore(DefaultSetup, parent)
{}

template <typename TType, typename TKey>
CachingDataTypeStore<TType, TKey>::CachingDataTypeStore(const QString &setupName, QObject *parent) :
	CachingDataTypeStore(new DataStore(setupName, nullptr), parent)
{
	_store->setParent(this);
}

template <typename TType, typename TKey>
CachingDataTypeStore<TType, TKey>::CachingDataTypeStore(DataStore *store, QObject *parent) :
	DataTypeStoreBase(parent),
	_store(store),
	_data()
{
	auto userProp = TType::staticMetaObject.userProperty();
	for(auto data : store->loadAll<TType>())
		_data.insert(userProp.readOnGadget(&data).template value<TKey>(), data);

	connect(_store, &DataStore::dataChanged,
			this, &CachingDataTypeStore::evalDataChanged);
	connect(_store, &DataStore::dataCleared,
			this, &CachingDataTypeStore::evalDataCleared);
	connect(_store, &DataStore::dataResetted,
			this, &CachingDataTypeStore::evalDataResetted);
}

template<typename TType, typename TKey>
DataStore *CachingDataTypeStore<TType, TKey>::store() const
{
	return _store;
}

template <typename TType, typename TKey>
qint64 CachingDataTypeStore<TType, TKey>::count() const
{
	return _data.size();
}

template <typename TType, typename TKey>
QList<TKey> CachingDataTypeStore<TType, TKey>::keys() const
{
	return _data.keys();
}

template<typename TType, typename TKey>
bool CachingDataTypeStore<TType, TKey>::contains(const TKey &key) const
{
	return _data.contains(key);
}

template <typename TType, typename TKey>
QList<TType> CachingDataTypeStore<TType, TKey>::loadAll() const
{
	return _data.values();
}

template <typename TType, typename TKey>
TType CachingDataTypeStore<TType, TKey>::load(const TKey &key) const
{
	return _data.value(key);
}

template <typename TType, typename TKey>
void CachingDataTypeStore<TType, TKey>::save(const TType &value)
{
	_store->save(value);
}

template <typename TType, typename TKey>
bool CachingDataTypeStore<TType, TKey>::remove(const TKey &key)
{
	return _store->remove<TType>(QVariant::fromValue(key).toString());
}

template<typename TType, typename TKey>
TType CachingDataTypeStore<TType, TKey>::take(const TKey &key)
{
	auto mData = _data.value(key);
	if(_store->remove<TType>(QVariant::fromValue(key).toString()))
		return mData;
	else
		return {};
}

template<typename TType, typename TKey>
void CachingDataTypeStore<TType, TKey>::clear()
{
	_store->clear<TType>();
}

template<typename TType, typename TKey>
typename CachingDataTypeStore<TType, TKey>::const_iterator CachingDataTypeStore<TType, TKey>::begin() const
{
	return _data.constBegin();
}

template<typename TType, typename TKey>
typename CachingDataTypeStore<TType, TKey>::const_iterator CachingDataTypeStore<TType, TKey>::end() const
{
	return _data.constEnd();
}

template<typename TType, typename TKey>
TKey CachingDataTypeStore<TType, TKey>::toKey(const QString &key)
{
	return QVariant(key).value<TKey>();
}

template <typename TType, typename TKey>
void CachingDataTypeStore<TType, TKey>::evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted)
{
	if(metaTypeId == qMetaTypeId<TType>()) {
		auto rKey = toKey(key);
		if(wasDeleted) {
			_data.remove(rKey);
			emit dataChanged(key, QVariant());
		} else {
			auto data = _store->load<TType>(key);
			_data.insert(rKey, data);
			emit dataChanged(key, QVariant::fromValue(data));
		}
	}
}

template <typename TType, typename TKey>
void CachingDataTypeStore<TType, TKey>::evalDataCleared(int metaTypeId)
{
	if(metaTypeId == qMetaTypeId<TType>())
		evalDataResetted();
}

template <typename TType, typename TKey>
void CachingDataTypeStore<TType, TKey>::evalDataResetted()
{
	_data.clear();
	emit dataResetted();
}

// ------------- GENERIC IMPLEMENTATION CachingDataTypeStore<TType*, TKey> -------------

template <typename TType, typename TKey>
CachingDataTypeStore<TType*, TKey>::CachingDataTypeStore(QObject *parent) :
	CachingDataTypeStore(DefaultSetup, parent)
{}

template <typename TType, typename TKey>
CachingDataTypeStore<TType*, TKey>::CachingDataTypeStore(const QString &setupName, QObject *parent) :
	CachingDataTypeStore(new DataStore(setupName, nullptr), parent)
{
	_store->setParent(this);
}

template <typename TType, typename TKey>
CachingDataTypeStore<TType*, TKey>::CachingDataTypeStore(DataStore *store, QObject *parent) :
	DataTypeStoreBase(parent),
	_store(store),
	_data()
{
	auto userProp = TType::staticMetaObject.userProperty();
	for(auto data : store->loadAll<TType*>()){
		data->setParent(this);
		_data.insert(userProp.read(data).template value<TKey>(), data);
	}

	connect(_store, &DataStore::dataChanged,
			this, &CachingDataTypeStore::evalDataChanged);
	connect(_store, &DataStore::dataCleared,
			this, &CachingDataTypeStore::evalDataCleared);
	connect(_store, &DataStore::dataResetted,
			this, &CachingDataTypeStore::evalDataResetted);
}

template<typename TType, typename TKey>
DataStore *CachingDataTypeStore<TType*, TKey>::store() const
{
	return _store;
}

template <typename TType, typename TKey>
qint64 CachingDataTypeStore<TType*, TKey>::count() const
{
	return _data.size();
}

template <typename TType, typename TKey>
QList<TKey> CachingDataTypeStore<TType*, TKey>::keys() const
{
	return _data.keys();
}

template<typename TType, typename TKey>
bool CachingDataTypeStore<TType *, TKey>::contains(const TKey &key) const
{
	return _data.contains(key);
}

template <typename TType, typename TKey>
QList<TType*> CachingDataTypeStore<TType*, TKey>::loadAll() const
{
	return _data.values();
}

template <typename TType, typename TKey>
TType *CachingDataTypeStore<TType*, TKey>::load(const TKey &key) const
{
	return _data.value(key);
}

template <typename TType, typename TKey>
void CachingDataTypeStore<TType*, TKey>::save(TType *value)
{
	_store->save(value);
}

template <typename TType, typename TKey>
bool CachingDataTypeStore<TType*, TKey>::remove(const TKey &key)
{
	return _store->remove<TType*>(key);
}

template<typename TType, typename TKey>
TType* CachingDataTypeStore<TType*, TKey>::take(const TKey &key)
{
	auto mData = _data.take(key);
	try {
		if(!_store->remove<TType*>(QVariant::fromValue(key).toString()))
			return nullptr;
	} catch(...) {
		_data.insert(key, mData);
		throw;
	}
	mData->setParent(nullptr);
	emit dataChanged(QVariant(key).toString(), QVariant());//manual emit required, because not happening in change signal because removed before
	return mData;
}

template<typename TType, typename TKey>
void CachingDataTypeStore<TType*, TKey>::clear()
{
	_store->clear<TType*>();
}

template<typename TType, typename TKey>
typename CachingDataTypeStore<TType*, TKey>::const_iterator CachingDataTypeStore<TType*, TKey>::begin() const
{
	return _data.constBegin();
}

template<typename TType, typename TKey>
typename CachingDataTypeStore<TType*, TKey>::const_iterator CachingDataTypeStore<TType*, TKey>::end() const
{
	return _data.constEnd();
}

template<typename TType, typename TKey>
TKey CachingDataTypeStore<TType*, TKey>::toKey(const QString &key)
{
	return QVariant(key).value<TKey>();
}

template <typename TType, typename TKey>
void CachingDataTypeStore<TType*, TKey>::evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted)
{
	if(metaTypeId == qMetaTypeId<TType*>()) {
		auto rKey = toKey(key);
		if(wasDeleted) {
			auto data = _data.take(rKey);
			if(data) {
				emit dataChanged(key, QVariant());
				data->deleteLater();
			}
		} else {
			if(_data.contains(rKey)) {
				_store->update(_data.value(rKey));
				emit dataChanged(key, QVariant::fromValue(_data.value(rKey)));
			} else {
				auto data = _store->load<TType*>(key);
				auto oldData = _data.take(rKey);
				data->setParent(this);
				_data.insert(rKey, data);
				emit dataChanged(key, QVariant::fromValue(data));
				if(oldData)
					oldData->deleteLater();
			}
		}
	}
}

template <typename TType, typename TKey>
void CachingDataTypeStore<TType*, TKey>::evalDataCleared(int metaTypeId)
{
	if(metaTypeId == qMetaTypeId<TType*>())
		evalDataResetted();
}

template <typename TType, typename TKey>
void CachingDataTypeStore<TType*, TKey>::evalDataResetted()
{
	auto data = _data;
	_data.clear();
	emit dataResetted();
	for(auto d : data)
		d->deleteLater();
}

}

#endif // QTDATASYNC_DATATYPESTORE_H
