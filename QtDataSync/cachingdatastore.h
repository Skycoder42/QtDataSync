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

template <typename T>
class CachingDataStore : public CachingDataStoreBase
{
public:
	explicit CachingDataStore(QObject *parent = nullptr, bool blockingConstruct = false);
	explicit CachingDataStore(const QString &setupName, QObject *parent = nullptr, bool blockingConstruct = false);

	int count() const;
	QStringList keys() const;
	QList<T> loadAll() const;
	T load(const QString &key) const;
	void save(const T &value);
	void remove(const QString &key);

private:
	AsyncDataStore *_store;
	QHash<QString, T> _data;

	void evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted);
};

// ------------- Generic Implementation -------------

template<typename T>
CachingDataStore<T>::CachingDataStore(QObject *parent, bool blockingConstruct) :
	CachingDataStore(Setup::DefaultSetup, parent, blockingConstruct)
{}

template<typename T>
CachingDataStore<T>::CachingDataStore(const QString &setupName, QObject *parent, bool blockingConstruct) :
	CachingDataStoreBase(parent),
	_store(new AsyncDataStore(setupName, this)),
	_data()
{
	auto metaTypeId = qMetaTypeId<T>();
	auto flags = QMetaType::typeFlags(metaTypeId);
	auto metaObject = QMetaType::metaObjectForType(metaTypeId);
	if((!flags.testFlag(QMetaType::PointerToQObject) &&
		!flags.testFlag(QMetaType::IsGadget)) ||
	   !metaObject)
		qFatal("You can only store QObjects or Q_GADGETs with QtDataSync!");

	auto resHandler = [=](const QList<T> &dataList){
		auto userProp = metaObject->userProperty();
		foreach(auto data, dataList) {
			QString key;
			if(flags.testFlag(QMetaType::PointerToQObject))
				key = userProp.read(reinterpret_cast<QObject*>(data)).toString();
			else
				key = userProp.readOnGadget(reinterpret_cast<void*>(data)).toString();
			_data.insert(key, data);
		}

		emit storeLoaded();
	};

	if(blockingConstruct)
		resHandler(_store->loadAll<T>().result());
	else
		_store->loadAll<T>().onResult(resHandler);

	connect(_store, &AsyncDataStore::dataChanged,
			this, &CachingDataStore::evalDataChanged);
}

template<typename T>
int CachingDataStore<T>::count() const
{
	return _data.size();
}

template<typename T>
QStringList CachingDataStore<T>::keys() const
{
	return _data.keys();
}

template<typename T>
QList<T> CachingDataStore<T>::loadAll() const
{
	return _data.values();
}

template<typename T>
T CachingDataStore<T>::load(const QString &key) const
{
	return _data.value(key);
}

template<typename T>
void CachingDataStore<T>::save(const T &value)
{
	auto metaTypeId = qMetaTypeId<T>();
	auto flags = QMetaType::typeFlags(metaTypeId);
	auto metaObject = QMetaType::metaObjectForType(metaTypeId);
	auto userProp = metaObject->userProperty();
	QString key;
	if(flags.testFlag(QMetaType::PointerToQObject))
		key = userProp.read(reinterpret_cast<QObject*>(value)).toString();
	else
		key = userProp.readOnGadget(reinterpret_cast<void*>(value)).toString();
	_data.insert(key, value);

	_store->save(value);
}

template<typename T>
void CachingDataStore<T>::remove(const QString &key)
{
	_data.remove(key);
	_store->remove<T>(key);
}

template<typename T>
void CachingDataStore<T>::evalDataChanged(int metaTypeId, const QString &key, bool wasDeleted)
{
	if(metaTypeId == qMetaTypeId<T>()) {
		if(wasDeleted) {
			_data.remove(key);
			emit dataChanged(key, true);
		} else {
			_store->load<T>(key).onResult([=](const T &data){
				_data.insert(key, data);
				emit dataChanged(key, false);
			});
		}
	}
}

}

#endif // CACHINGDATASTORE_H
