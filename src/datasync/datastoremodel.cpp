#include "datastoremodel.h"
#include "datastoremodel_p.h"
using namespace QtDataSync;

#define FN d->fn

DataStoreModel::DataStoreModel(QObject *parent) :
	QAbstractListModel(parent),
	d(new DataStoreModelPrivate(this, new AsyncDataStore(this)))
{}

DataStoreModel::DataStoreModel(const QString &setupName, QObject *parent) :
	QAbstractListModel(parent),
	d(new DataStoreModelPrivate(this, new AsyncDataStore(setupName, this)))
{}

DataStoreModel::DataStoreModel(AsyncDataStore *store, QObject *parent) :
	QAbstractListModel(parent),
	d(new DataStoreModelPrivate(this, store))
{}

DataStoreModel::~DataStoreModel() {}

AsyncDataStore *DataStoreModel::store() const
{
	return d->store;
}

int DataStoreModel::typeId() const
{
	return d->type;
}

bool DataStoreModel::isEditable() const
{
	return d->editable;
}

bool DataStoreModel::setTypeId(int typeId)
{
	auto flags = QMetaType::typeFlags(typeId);
	if(flags.testFlag(QMetaType::IsGadget) ||
	   flags.testFlag(QMetaType::PointerToQObject)) {
		beginResetModel();
		d->type = typeId;
		d->isObject = flags.testFlag(QMetaType::PointerToQObject);
		d->keyList.clear();
		d->clearHashObjects();
		d->createRoleNames();
		endResetModel();

		d->store->loadAll(typeId).onResult(this, FN(&DataStoreModelPrivate::onLoadAll), FN(&DataStoreModelPrivate::onError));
		return true;
	} else
		return false;
}

void DataStoreModel::setEditable(bool editable)
{
	if (d->editable == editable)
		return;

	d->editable = editable;
	emit editableChanged(editable);
}

QVariant DataStoreModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	auto metaObject = QMetaType::metaObjectForType(d->type);
	if(metaObject && section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return QString::fromUtf8(metaObject->className());
	else
		return {};
}

int DataStoreModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	else
		return d->keyList.size();
}

QModelIndex DataStoreModel::idIndex(const QString &id) const
{
	auto idx = d->keyList.indexOf(id);
	if(idx != -1)
		return index(idx);
	else
		return {};
}

QString DataStoreModel::key(const QModelIndex &index) const
{
	if(index.isValid() &&
	   index.column() == 0 &&
	   index.row() < d->keyList.size())
		return d->keyList[index.row()];
	else
		return {};
}

QVariant DataStoreModel::data(const QModelIndex &index, int role) const
{
	if (!d->testValid(index, role))
		return {};

	return d->readProperty(key(index), d->roleNames.value(role));
}

bool DataStoreModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!d->editable || !d->testValid(index, role))
		return {};

	if(d->writeProperty(key(index), d->roleNames.value(role), value)) {
		emit dataChanged(index, index, {role});
		return true;
	} else
		return false;
}

QVariant DataStoreModel::object(const QModelIndex &index) const
{
	auto iKey = key(index);
	if(iKey.isNull())
		return {};
	else
		return d->dataHash.value(iKey);
}

Task DataStoreModel::loadObject(const QModelIndex &index) const
{
	auto iKey = key(index);
	if(iKey.isNull()) {
		QFutureInterface<QVariant> d;
		d.reportStarted();
		d.reportException(DataSyncException("QModelIndex is not valid"));
		d.reportFinished();
		return GenericTask<void>(d);
	} else
		return d->store->load(d->type, iKey);
}

Qt::ItemFlags DataStoreModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	auto flags = Qt::ItemIsEnabled |
			Qt::ItemIsSelectable |
			Qt::ItemNeverHasChildren;
	if(d->editable)
		flags |= Qt::ItemIsEditable;
	return flags;
}

QHash<int, QByteArray> DataStoreModel::roleNames() const
{
	return d->roleNames;
}

void DataStoreModel::storeChanged(int metaTypeId, const QString &key, bool wasDeleted)
{
	if(metaTypeId != d->type)
		return;

	if(wasDeleted) {
		auto index = d->keyList.indexOf(key);
		if(index != -1) {
			beginRemoveRows(QModelIndex(), index, index);
			d->keyList.removeAt(index);
			d->deleteObject(d->dataHash.take(key));
			endRemoveRows();
		}
	} else
		d->store->load(d->type, key).onResult(this, FN(&DataStoreModelPrivate::onLoad), FN(&DataStoreModelPrivate::onError));
}

void DataStoreModel::storeResetted()
{
	beginResetModel();
	d->keyList.clear();
	d->clearHashObjects();
	endResetModel();
}

// ------------- Private Implementation -------------

#undef FN
#define FN fn

DataStoreModelPrivate::DataStoreModelPrivate(DataStoreModel *q_ptr, AsyncDataStore *store) :
	q(q_ptr),
	store(store),
	editable(false),
	type(QMetaType::UnknownType),
	isObject(false),
	roleNames(),
	keyList(),
	dataHash()
{
	QObject::connect(store, &AsyncDataStore::dataChanged,
					 q, &DataStoreModel::storeChanged);
	QObject::connect(store, &AsyncDataStore::dataResetted,
					 q, &DataStoreModel::storeResetted);
}

void DataStoreModelPrivate::createRoleNames()
{
	roleNames.clear();

	auto metaObject = QMetaType::metaObjectForType(type);
	roleNames.insert(Qt::DisplayRole, metaObject->userProperty().name());//use the key for the display role

	auto roleIndex = Qt::UserRole + 1;
	for(auto i = 0; i < metaObject->propertyCount(); i++) {
		auto prop = metaObject->property(i);
		if(!prop.isUser())
			roleNames.insert(roleIndex++, prop.name());
	}
}

void DataStoreModelPrivate::clearHashObjects()
{
	if(QMetaType::typeFlags(type).testFlag(QMetaType::PointerToQObject)) {
		foreach(auto v, dataHash)
			deleteObject(v);
	}
	dataHash.clear();
}

void DataStoreModelPrivate::deleteObject(const QVariant &value)
{
	auto obj = value.value<QObject*>();
	if(obj)
		obj->deleteLater();
}

bool DataStoreModelPrivate::testValid(const QModelIndex &index, int role) const
{
	return index.isValid() &&
			index.column() == 0 &&
			index.row() < keyList.size() &&
			(role < 0 || roleNames.contains(role));
}

void DataStoreModelPrivate::onLoadAll(QVariant data)
{
	auto listEmpty = keyList.isEmpty();
	q->beginResetModel();
	foreach(auto v, data.toList()) {
		auto key = readKey(v);
		if(!key.isNull()) {
			dataHash.insert(key, v);
			if(listEmpty || !keyList.contains(key))
				keyList.append(key);
		}
	}
	q->endResetModel();

	emit q->storeLoaded();
}

void DataStoreModelPrivate::onLoad(QVariant data)
{
	auto key = readKey(data);
	if(key.isNull())
		return;

	auto index = keyList.indexOf(key);
	if(index == -1) {
		q->beginInsertRows(QModelIndex(), keyList.size(), keyList.size());
		keyList.append(key);
		dataHash.insert(key, data);
		q->endInsertRows();
	} else {
		auto oldData = dataHash.take(key);
		dataHash.insert(key, data);
		auto mIndex = q->index(index);
		emit q->dataChanged(mIndex, mIndex);
		deleteObject(oldData);
	}
}

void DataStoreModelPrivate::onError(const QException &exception)
{
	emit q->storeError(exception);
}

QString DataStoreModelPrivate::readKey(QVariant data)
{
	if(!data.convert(type))
		return {};

	auto metaObject = QMetaType::metaObjectForType(type);
	auto userProp = metaObject->userProperty();
	if(isObject) {
		auto object = data.value<QObject*>();
		if(object)
			return userProp.read(object).toString();
		else
			return {};
	} else
		return userProp.readOnGadget(data.constData()).toString();
}

QVariant DataStoreModelPrivate::readProperty(const QString &key, const QByteArray &property)
{
	auto data = dataHash.value(key);
	if(!data.convert(type))
		return {};

	auto metaObject = QMetaType::metaObjectForType(type);
	auto pIndex = metaObject->indexOfProperty(property.constData());
	if(pIndex == -1)
		return {};
	auto prop = metaObject->property(pIndex);

	if(isObject) {
		auto object = data.value<QObject*>();
		if(object)
			return prop.read(object).toString();
		else
			return {};
	} else
		return prop.readOnGadget(data.constData()).toString();
}

bool DataStoreModelPrivate::writeProperty(const QString &key, const QByteArray &property, const QVariant &value)
{
	auto data = dataHash.value(key);
	if(!data.convert(type))
		return false;

	auto metaObject = QMetaType::metaObjectForType(type);
	auto pIndex = metaObject->indexOfProperty(property.constData());
	if(pIndex == -1)
		return false;
	auto prop = metaObject->property(pIndex);
	if(prop.isUser())//user property not editable, as this would change the identity
		return false;

	if(isObject) {
		auto object = data.value<QObject*>();
		if(object)
			prop.write(object, value);
		else
			return false;
	} else
		prop.writeOnGadget(data.data(), value);

	dataHash[key] = data;
	store->save(type, data).onResult(q, {}, FN(&DataStoreModelPrivate::onError));
	return true;
}
