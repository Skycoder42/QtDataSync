#ifndef CACHINGDATASTORE_H
#define CACHINGDATASTORE_H

#include "asyncdatastore.h"
#include "qtdatasync_global.h"
#include "setup.h"
#include <QObject>

namespace QtDataSync {

class QTDATASYNCSHARED_EXPORT CachingDataStoreBase : public QObject
{
	Q_OBJECT

public:
	explicit CachingDataStoreBase(QObject *parent = nullptr);

signals:
	void storeLoaded();
	void dataChanged(const QString &key, bool wasDeleted);
};

template <typename TType, typename TKey = QString>
class CachingDataStore : public CachingDataStoreBase
{
public:
	explicit CachingDataStore(QObject *parent = nullptr, bool blockingConstruct = false);
	explicit CachingDataStore(const QString &setupName, QObject *parent = nullptr, bool blockingConstruct = false);

	int count() const;
	QList<TKey> keys() const;
	QList<TType> loadAll() const;
	TType load(const TKey &key) const;
	void save(const TType &value);
	void remove(const TKey &key);

	TKey toKey(const QString &key) const;

private:
	AsyncDataStore *_store;
	QHash<TKey, TType> _data;

	void evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted);
};

template <typename TType, typename TKey>
class CachingDataStore<TType*, TKey> : public CachingDataStoreBase
{
	static_assert(std::is_base_of<QObject, TType>::value, "TType must inherit QObject!");
public:
	explicit CachingDataStore(QObject *parent = nullptr, bool blockingConstruct = false);
	explicit CachingDataStore(const QString &setupName, QObject *parent = nullptr, bool blockingConstruct = false);

	int count() const;
	QList<TKey> keys() const;
	QList<TType*> loadAll() const;
	TType* load(const TKey &key) const;
	void save(TType *value);
	void remove(const TKey &key);

	TKey toKey(const QString &key) const;

private:
	AsyncDataStore *_store;
	QHash<TKey, TType*> _data;

	void evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted);
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
		_store->loadAll<TType>().onResult(resHandler);

	connect(_store, &AsyncDataStore::dataChanged,
			this, &CachingDataStore::evalDataChanged);
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
			_store->load<TType>(key).onResult([=](const TType &data){
				_data.insert(rKey, data);
				emit dataChanged(key, false);
			});
		}
	}
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
		_store->loadAll<TType*>().onResult(resHandler);

	connect(_store, &AsyncDataStore::dataChanged,
			this, &CachingDataStore::evalDataChanged);
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
			_store->load<TType*>(key).onResult([=](TType* data){
				data->setParent(this);
				_data.insert(rKey, data);
				emit dataChanged(key, false);
			});
		}
	}
}

}

#endif // CACHINGDATASTORE_H
