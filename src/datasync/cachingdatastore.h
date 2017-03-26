#ifndef CACHINGDATASTORE_H
#define CACHINGDATASTORE_H

#include "QtDataSync/qdatasync_global.h"
#include "QtDataSync/asyncdatastore.h"
#include "QtDataSync/setup.h"

#include <QtCore/qobject.h>
#include <functional>

namespace QtDataSync {

//! Base class for CachingDataStore, to provide signals
class Q_DATASYNC_EXPORT CachingDataStoreBase : public QObject
{
	Q_OBJECT

public:
	//! Constructor
	explicit CachingDataStoreBase(QObject *parent = nullptr);

Q_SIGNALS:
	//! Will be emitted once, after the initial data was loaded
	void storeLoaded();
	//! Will be emitted when a dataset in the store has changed
	void dataChanged(const QString &key, bool wasDeleted);
	//! Will be emitted when the store has be reset (cleared)
	void dataResetted();
};

//! A class to access data synchronously
template <typename TType, typename TKey = QString>
class CachingDataStore : public CachingDataStoreBase
{
public:
	//! Constructs a store for the default setup
	explicit CachingDataStore(QObject *parent = nullptr, bool blockingConstruct = false);
	//! Constructs a store for the given setup
	explicit CachingDataStore(const QString &setupName, QObject *parent = nullptr, bool blockingConstruct = false);

	//! Counts the number of datasets in the store
	int count() const;
	//! Returns all keys in the store
	QList<TKey> keys() const;
	//! Loads all existing datasets
	QList<TType> loadAll() const;
	//! Loads the dataset with the given key
	TType load(const TKey &key) const;
	//! Saves the given dataset
	void save(const TType &value);
	//! Removes the dataset with the given key
	void remove(const TKey &key);

	//! Shortcut to convert a string to the stores key typee
	TKey toKey(const QString &key) const;

private:
	AsyncDataStore *_store;
	QHash<TKey, TType> _data;

	void evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void evalDataResetted();
};

/*!
@brief CachingDataStore specialization for QObject* types
@details The main difference of the QObject specialization is the fact, that all QObject
instances are owned by the store. This means saving will reparent objects to the store, and
remove will delete them etc.

@copydetails QtDataSync::CachingDataStore
*/
template <typename TType, typename TKey>
class CachingDataStore<TType*, TKey> : public CachingDataStoreBase
{
	static_assert(std::is_base_of<QObject, TType>::value, "TType must inherit QObject!");
public:
	//!@copydoc CachingDataStore::CachingDataStore(QObject *, bool)
	explicit CachingDataStore(QObject *parent = nullptr, bool blockingConstruct = false);
	//!@copydoc CachingDataStore::CachingDataStore(const QString &, QObject *, bool)
	explicit CachingDataStore(const QString &setupName, QObject *parent = nullptr, bool blockingConstruct = false);

	//!@copydoc CachingDataStore::count
	int count() const;
	//!@copydoc CachingDataStore::keys
	QList<TKey> keys() const;
	//!@copydoc CachingDataStore::loadAll
	QList<TType*> loadAll() const;
	//!@copydoc CachingDataStore::load
	TType* load(const TKey &key) const;
	//!@copydoc CachingDataStore::save
	void save(TType *value);
	//!@copydoc CachingDataStore::remove
	void remove(const TKey &key);

	//!@copydoc CachingDataStore::toKey
	TKey toKey(const QString &key) const;

private:
	AsyncDataStore *_store;
	QHash<TKey, TType*> _data;

	void evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void evalDataResetted();
};

// ------------- Generic Implementation -------------

template <typename TType, typename TKey>
CachingDataStore<TType, TKey>::CachingDataStore(QObject *parent, bool blockingConstruct) :
	CachingDataStore(Setup::DefaultSetup, parent, blockingConstruct)
{}

template <typename TType, typename TKey>
CachingDataStore<TType, TKey>::CachingDataStore(const QString &setupName, QObject *parent, bool blockingConstruct) :
	CachingDataStoreBase(parent),
	_store(new AsyncDataStore(setupName, this)),
	_data()
{
	auto metaTypeId = qMetaTypeId<TType>();
	auto flags = QMetaType::typeFlags(metaTypeId);
	if((!flags.testFlag(QMetaType::IsGadget)))
		qFatal("You can only store QObjects or Q_GADGETs with QtDataSync! Q_GADGETS are supported as values, QObjects as pointers");

	auto resHandler = [=](const QList<TType> &dataList){
		auto userProp = TType::staticMetaObject.userProperty();
		foreach(auto data, dataList)
			_data.insert(userProp.readOnGadget(&data).template value<TKey>(), data);
		emit storeLoaded();
	};

	if(blockingConstruct)
		resHandler(_store->loadAll<TType>().result());
	else
		_store->loadAll<TType>().onResult(this, resHandler);

	connect(_store, &AsyncDataStore::dataChanged,
			this, &CachingDataStore::evalDataChanged);
	connect(_store, &AsyncDataStore::dataResetted,
			this, &CachingDataStore::evalDataResetted);
}

template <typename TType, typename TKey>
int CachingDataStore<TType, TKey>::count() const
{
	return _data.size();
}

template <typename TType, typename TKey>
QList<TKey> CachingDataStore<TType, TKey>::keys() const
{
	return _data.keys();
}

template <typename TType, typename TKey>
QList<TType> CachingDataStore<TType, TKey>::loadAll() const
{
	return _data.values();
}

template <typename TType, typename TKey>
TType CachingDataStore<TType, TKey>::load(const TKey &key) const
{
	return _data.value(key);
}

template <typename TType, typename TKey>
void CachingDataStore<TType, TKey>::save(const TType &value)
{
	auto userProp = TType::staticMetaObject.userProperty();
	_data.insert(userProp.readOnGadget(&value).template value<TKey>(), value);
	_store->save(value);
}

template <typename TType, typename TKey>
void CachingDataStore<TType, TKey>::remove(const TKey &key)
{
	_data.remove(key);
	_store->remove<TType>(QVariant::fromValue(key).toString());
}

template<typename TType, typename TKey>
TKey CachingDataStore<TType, TKey>::toKey(const QString &key) const
{
	return QVariant(key).value<TKey>();
}

template <typename TType, typename TKey>
void CachingDataStore<TType, TKey>::evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted)
{
	if(metaTypeId == qMetaTypeId<TType>()) {
		auto rKey = toKey(key);
		if(wasDeleted) {
			_data.remove(rKey);
			emit dataChanged(key, true);
		} else {
			_store->load<TType>(key).onResult(this, [=](const TType &data){
				_data.insert(rKey, data);
				emit dataChanged(key, false);
			});
		}
	}
}

template <typename TType, typename TKey>
void CachingDataStore<TType, TKey>::evalDataResetted()
{
	_data.clear();
	emit dataResetted();
}

// ------------- Generic Implementation specialisation -------------

template <typename TType, typename TKey>
CachingDataStore<TType*, TKey>::CachingDataStore(QObject *parent, bool blockingConstruct) :
	CachingDataStore(Setup::DefaultSetup, parent, blockingConstruct)
{}

template <typename TType, typename TKey>
CachingDataStore<TType*, TKey>::CachingDataStore(const QString &setupName, QObject *parent, bool blockingConstruct) :
	CachingDataStoreBase(parent),
	_store(new AsyncDataStore(setupName, this)),
	_data()
{
	auto resHandler = [=](const QList<TType*> &dataList){
		auto userProp = TType::staticMetaObject.userProperty();
		foreach(auto data, dataList) {
			data->setParent(this);
			_data.insert(userProp.read(data).template value<TKey>(), data);
		}
		emit storeLoaded();
	};

	if(blockingConstruct)
		resHandler(_store->loadAll<TType*>().result());
	else
		_store->loadAll<TType*>().onResult(this, resHandler);

	connect(_store, &AsyncDataStore::dataChanged,
			this, &CachingDataStore::evalDataChanged);
	connect(_store, &AsyncDataStore::dataResetted,
			this, &CachingDataStore::evalDataResetted);
}

template <typename TType, typename TKey>
int CachingDataStore<TType*, TKey>::count() const
{
	return _data.size();
}

template <typename TType, typename TKey>
QList<TKey> CachingDataStore<TType*, TKey>::keys() const
{
	return _data.keys();
}

template <typename TType, typename TKey>
QList<TType*> CachingDataStore<TType*, TKey>::loadAll() const
{
	return _data.values();
}

template <typename TType, typename TKey>
TType *CachingDataStore<TType*, TKey>::load(const TKey &key) const
{
	return _data.value(key);
}

template <typename TType, typename TKey>
void CachingDataStore<TType*, TKey>::save(TType *value)
{
	auto userProp = TType::staticMetaObject.userProperty();
	value->setParent(this);
	_data.insert(userProp.read(value).template value<TKey>(), value);
	_store->save(value);
}

template <typename TType, typename TKey>
void CachingDataStore<TType*, TKey>::remove(const TKey &key)
{
	auto data = _data.take(key);
	data->deleteLater();
	_store->remove<TType*>(QVariant::fromValue(key).toString());
}

template<typename TType, typename TKey>
TKey CachingDataStore<TType*, TKey>::toKey(const QString &key) const
{
	return QVariant(key).value<TKey>();
}

template <typename TType, typename TKey>
void CachingDataStore<TType*, TKey>::evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted)
{
	if(metaTypeId == qMetaTypeId<TType*>()) {
		auto rKey = toKey(key);
		if(wasDeleted) {
			auto data = _data.take(rKey);
			data->deleteLater();
			emit dataChanged(key, true);
		} else {
			_store->load<TType*>(key).onResult(this, [=](TType* data){
				data->setParent(this);
				_data.insert(rKey, data);
				emit dataChanged(key, false);
			});
		}
	}
}

template <typename TType, typename TKey>
void CachingDataStore<TType*, TKey>::evalDataResetted()
{
	_data.clear();
	emit dataResetted();
}

}

#endif // CACHINGDATASTORE_H
