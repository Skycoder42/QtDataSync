#include "datastoremodel.h"
#include "datastoremodel_p.h"
#include "datastore_p.h"

#include <QtCore/QMetaProperty>

using namespace QtDataSync;

DataStoreModel::DataStoreModel(QObject *parent) :
	DataStoreModel(DefaultSetup, parent)
{}

DataStoreModel::DataStoreModel(const QString &setupName, QObject *parent) :
	QAbstractListModel(parent),
	d(new DataStoreModelPrivate(this))
{
	initStore(new DataStore(setupName));
	d->store->setParent(this);
}

DataStoreModel::DataStoreModel(DataStore *store, QObject *parent) :
	QAbstractListModel(parent),
	d(new DataStoreModelPrivate(this))
{
	initStore(store);
}

DataStoreModel::DataStoreModel(QObject *parent, void *):
	QAbstractListModel(parent),
	d(new DataStoreModelPrivate(this))
{} //No init

void DataStoreModel::initStore(DataStore *store)
{
	d->store = store;
	QObject::connect(d->store, &DataStore::dataChanged,
					 this, &DataStoreModel::storeChanged);
	QObject::connect(d->store, &DataStore::dataCleared,
					 this, &DataStoreModel::storeCleared);
	QObject::connect(d->store, &DataStore::dataResetted,
					 this, &DataStoreModel::storeResetted);
}

DataStoreModel::~DataStoreModel() {}

DataStore *DataStoreModel::store() const
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
		return d->dataHash.size();
}

bool DataStoreModel::canFetchMore(const QModelIndex &parent) const
{
	if(parent.isValid())
		return false;
	else
		return d->dataHash.size() < d->keyList.size();
}

void DataStoreModel::fetchMore(const QModelIndex &parent)
{
	if(!parent.isValid()) {
		try {
			//load 100 at once
			auto offset = d->dataHash.size();
			auto max = qMin(offset + 100, d->keyList.size());
			QVariantHash loadData;

			for(auto i = offset; i < max; i++) {
				auto key = d->keyList.value(i);
				loadData.insert(key, d->store->load(d->type, key));
			}

			beginInsertRows(parent, offset, max - 1);
			d->dataHash.unite(loadData);//no duplicates thanks to logic
			endInsertRows();
		} catch(QException &e) {
			emit storeError(e, {});
		}
	}
}

QModelIndex DataStoreModel::idIndex(const QString &id) const
{
	auto idx = d->activeKeys().indexOf(id);
	if(idx != -1)
		return index(idx);
	else
		return {};
}

QString DataStoreModel::key(const QModelIndex &index) const
{
	auto active = d->activeKeys();
	if(index.isValid() &&
	   index.column() == 0 &&
	   index.row() < active.size())
		return active[index.row()];
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

QVariant DataStoreModel::loadObject(const QModelIndex &index) const
{
	auto iKey = key(index);
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

void DataStoreModel::setTypeId(int typeId)
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

		try {
			d->keyList = d->store->keys(typeId);
			endResetModel();
		} catch(...) {
			endResetModel();
			throw;
		}
	} else
		throw InvalidDataException(d->store->d->defaults, QMetaType::typeName(typeId), QStringLiteral("Type is neither a gadget nor a pointer to an object"));
}

void DataStoreModel::setEditable(bool editable)
{
	if (d->editable == editable)
		return;

	d->editable = editable;
	emit editableChanged(editable, {});
}

void DataStoreModel::reload()
{
	beginResetModel();
	d->keyList.clear();
	d->clearHashObjects();
	try {
		d->keyList = d->store->keys(d->type);
		endResetModel();
	} catch(QException &e) {
		endResetModel();
		emit storeError(e, {});
	}
}

void DataStoreModel::storeChanged(int metaTypeId, const QString &key, bool wasDeleted)
{
	if(metaTypeId != d->type)
		return;

	if(wasDeleted) {
		auto index = d->keyList.indexOf(key);
		if(index != -1) {
			if(index < d->dataHash.size()) { //is already fetched
				beginRemoveRows(QModelIndex(), index, index);
				d->keyList.removeAt(index);
				d->deleteObject(d->dataHash.take(key));
				endRemoveRows();
			} else //not fetched yet -> no signals needed
				d->keyList.removeAt(index);
		} //else no need to remove something already not existing
	} else {
		auto index = d->keyList.indexOf(key);
		if(index != -1) { //key already know
			if(index < d->dataHash.size()) { //not fully loaded -> only load if already fetched
				try {
					if(d->isObject) {
						auto obj = d->dataHash.value(key).value<QObject*>();
						d->store->update(d->type, obj);
					} else
						d->dataHash.insert(key, d->store->load(d->type, key));
					auto mIndex = idIndex(key);
					emit dataChanged(mIndex, mIndex);
				} catch(QException &e) {
					emit storeError(e, {});
				}
			}
		} else { //key unknown -> append it
			if(d->keyList.size() == d->dataHash.size()) { //already fully loaded -> needs to be loaded as well
				d->keyList.append(key);
				fetchMore(QModelIndex());//simply call fetch more does the loading
			} else //only append
				d->keyList.append(key);
		}
	}
}

void DataStoreModel::storeCleared(int metaTypeId)
{
	if(d->type == metaTypeId)
		storeResetted();
}

void DataStoreModel::storeResetted()
{
	beginResetModel();
	d->keyList.clear();
	d->clearHashObjects();
	endResetModel();
}

// ------------- Private Implementation -------------

DataStoreModelPrivate::DataStoreModelPrivate(DataStoreModel *q_ptr) :
	q(q_ptr),
	store(nullptr),
	editable(false),
	type(QMetaType::UnknownType),
	isObject(false),
	roleNames(),
	keyList(),
	dataHash()
{}

QStringList DataStoreModelPrivate::activeKeys()
{
	return keyList.mid(0, dataHash.size());
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
		for(auto v : dataHash)
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
	store->save(type, data);
	return true;
}

