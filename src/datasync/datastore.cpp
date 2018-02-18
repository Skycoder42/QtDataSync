#include "datastore.h"
#include "datastore_p.h"
#include "defaults_p.h"

#include <QtJsonSerializer/QJsonSerializer>

#include "signal_private_connect_p.h"

using namespace QtDataSync;
using std::function;

DataStore::DataStore(QObject *parent) :
	DataStore(DefaultSetup, parent)
{}

DataStore::DataStore(const QString &setupName, QObject *parent) :
	QObject(parent),
	d(nullptr)
{
	initStore(setupName);
}

DataStore::DataStore(QObject *parent, void *) :
	QObject(parent),
	d(nullptr)
{}

void DataStore::initStore(const QString &setupName)
{
	d.reset(new DataStorePrivate(this, setupName));
	connect(d->store, &LocalStore::dataChanged,
			this, [this](const ObjectKey &key, bool deleted) {
		emit dataChanged(QMetaType::type(key.typeName), key.id, deleted, {});
	});
	connect(d->store, &LocalStore::dataCleared,
			this, [this](const QByteArray &typeName) {
		emit dataCleared(QMetaType::type(typeName), {});
	});
	connect(d->store, &LocalStore::dataResetted,
			this, PSIG(&DataStore::dataResetted));
}

DataStore::~DataStore() {}

qint64 DataStore::count(int metaTypeId) const
{
	return d->store->count(d->typeName(metaTypeId));
}

QStringList DataStore::keys(int metaTypeId) const
{
	return d->store->keys(d->typeName(metaTypeId));
}

QVariantList DataStore::loadAll(int metaTypeId) const
{
	QVariantList resList;
	for(auto val : d->store->loadAll(d->typeName(metaTypeId)))
		resList.append(d->serializer->deserialize(val, metaTypeId));
	return resList;
}

QVariant DataStore::load(int metaTypeId, const QString &key) const
{
	auto data = d->store->load({d->typeName(metaTypeId), key});
	return d->serializer->deserialize(data, metaTypeId);
}

void DataStore::save(int metaTypeId, QVariant value)
{
	auto typeName = d->typeName(metaTypeId);
	if(!value.convert(metaTypeId))
		throw InvalidDataException(d->defaults, typeName, QStringLiteral("Failed to convert passed variant to the target type"));

	auto meta = QMetaType::metaObjectForType(metaTypeId);
	if(!meta)
		throw InvalidDataException(d->defaults, typeName, QStringLiteral("Type does not have a meta object"));
	auto userProp = meta->userProperty();
	if(!userProp.isValid())
		throw InvalidDataException(d->defaults, typeName, QStringLiteral("Type does not have a user property"));

	QString key;
	auto flags = QMetaType::typeFlags(metaTypeId);
	if(flags.testFlag(QMetaType::IsGadget))
		key = userProp.readOnGadget(value.data()).toString();
	else if(flags.testFlag(QMetaType::PointerToQObject))
		key = userProp.read(value.value<QObject*>()).toString();
	else if(flags.testFlag(QMetaType::SharedPointerToQObject))
		key = userProp.read(value.value<QSharedPointer<QObject>>().data()).toString();
	else if(flags.testFlag(QMetaType::WeakPointerToQObject))
		key = userProp.read(value.value<QWeakPointer<QObject>>().data()).toString();
	else if(flags.testFlag(QMetaType::TrackingPointerToQObject))
		key = userProp.read(value.value<QPointer<QObject>>().data()).toString();
	else
		throw InvalidDataException(d->defaults, typeName, QStringLiteral("Type is neither a gadget nor a pointer to an object"));

	if(key.isEmpty())
		throw InvalidDataException(d->defaults, typeName, QStringLiteral("Failed to convert USER property to a string"));
	auto json = d->serializer->serialize(value);
	if(!json.isObject())
		throw InvalidDataException(d->defaults, typeName, QStringLiteral("Serialization converted to invalid json type. Only json objects are allowed"));
	d->store->save({typeName, key}, json.toObject());
}

bool DataStore::remove(int metaTypeId, const QString &key)
{
	return d->store->remove({d->typeName(metaTypeId), key});
}

void DataStore::update(int metaTypeId, QObject *object) const
{
	auto typeName = d->typeName(metaTypeId);
	auto meta = QMetaType::metaObjectForType(metaTypeId);
	if(!meta)
		throw InvalidDataException(d->defaults, typeName, QStringLiteral("Type does not have a meta object"));
	auto userProp = meta->userProperty();
	if(!userProp.isValid())
		throw InvalidDataException(d->defaults, typeName, QStringLiteral("Type does not have a user property"));

	auto key = userProp.read(object).toString();
	if(key.isEmpty())
		throw InvalidDataException(d->defaults, typeName, QStringLiteral("Failed to convert user property value to a string"));

	if(!object->metaObject()->inherits(meta)) {
		throw InvalidDataException(d->defaults,
								   typeName,
								   QStringLiteral("Passed object of type %1 does not inherit the given meta type")
								   .arg(QString::fromUtf8(object->metaObject()->className())));
	}

	auto nObj = load(metaTypeId, key).value<QObject*>();
	for(auto i = 0; i < meta->propertyCount(); i++) {
		auto prop = meta->property(i);
		prop.write(object, prop.read(nObj));
	}
	nObj->deleteLater();
}

QVariantList DataStore::search(int metaTypeId, const QString &query, SearchMode mode) const
{
	QVariantList resList;
	for(auto val : d->store->find(d->typeName(metaTypeId), query, mode))
		resList.append(d->serializer->deserialize(val, metaTypeId));
	return resList;
}

void DataStore::iterate(int metaTypeId, const function<bool (QVariant)> &iterator) const
{
	for(auto key : keys(metaTypeId)) {
		if(!iterator(load(metaTypeId, key)))
			break;
	}
}

void DataStore::clear(int metaTypeId)
{
	d->store->clear(d->typeName(metaTypeId));
}

// ------------- PRIVATE IMPLEMENTATION -------------

DataStorePrivate::DataStorePrivate(DataStore *q, const QString &setupName) :
	defaults(DefaultsPrivate::obtainDefaults(setupName)),
	logger(defaults.createLogger("datastore", q)),
	serializer(defaults.serializer()),
	store(new LocalStore(defaults, q))
{}

QByteArray DataStorePrivate::typeName(int metaTypeId) const
{
	auto name = QMetaType::typeName(metaTypeId);
	if(name)
		return name;
	else
		throw InvalidDataException(defaults, "type_" + QByteArray::number(metaTypeId), QStringLiteral("Not a valid metatype id"));
}

// ------------- Exceptions -------------

DataStoreException::DataStoreException(const Defaults &defaults, const QString &message) :
	Exception(defaults, message)
{}

DataStoreException::DataStoreException(const DataStoreException * const other) :
	Exception(other)
{}



LocalStoreException::LocalStoreException(const Defaults &defaults, const ObjectKey &key, const QString &context, const QString &message) :
	DataStoreException(defaults, message),
	_key(key),
	_context(context)
{}

LocalStoreException::LocalStoreException(const LocalStoreException * const other) :
	DataStoreException(other),
	_key(other->_key),
	_context(other->_context)
{}

ObjectKey LocalStoreException::key() const
{
	return _key;
}

QString LocalStoreException::context() const
{
	return _context;
}

QByteArray LocalStoreException::className() const noexcept
{
	return QTDATASYNC_EXCEPTION_NAME(LocalStoreException);
}

QString LocalStoreException::qWhat() const
{
	QString msg = Exception::qWhat() +
				  QStringLiteral("\n\tContext: %1"
								 "\n\tTypeName: %2")
				  .arg(_context)
				  .arg(QString::fromUtf8(_key.typeName));
	if(!_key.id.isNull())
		msg += QStringLiteral("\n\tKey: %1").arg(_key.id);
	return msg;
}

void LocalStoreException::raise() const
{
	throw (*this);
}

QException *LocalStoreException::clone() const
{
	return new LocalStoreException(this);
}



NoDataException::NoDataException(const Defaults &defaults, const ObjectKey &key) :
	DataStoreException(defaults, QStringLiteral("The requested data does not exist")),
	_key(key)
{}

NoDataException::NoDataException(const NoDataException * const other) :
	DataStoreException(other),
	_key(other->_key)
{}

ObjectKey NoDataException::key() const
{
	return _key;
}

QByteArray NoDataException::className() const noexcept
{
	return QTDATASYNC_EXCEPTION_NAME(NoDataException);
}

QString NoDataException::qWhat() const
{
	return Exception::qWhat() +
			QStringLiteral("\n\tTypeName: %1"
						   "\n\tKey: %2")
			.arg(QString::fromUtf8(_key.typeName))
			.arg(_key.id);
}

void NoDataException::raise() const
{
	throw (*this);
}

QException *NoDataException::clone() const
{
	return new NoDataException(this);
}



InvalidDataException::InvalidDataException(const Defaults &defaults, const QByteArray &typeName, const QString &message) :
	DataStoreException(defaults, message),
	_typeName(typeName)
{}

InvalidDataException::InvalidDataException(const InvalidDataException * const other) :
	DataStoreException(other),
	_typeName(other->_typeName)
{}

QByteArray InvalidDataException::typeName() const
{
	return _typeName;
}

QByteArray InvalidDataException::className() const noexcept
{
	return QTDATASYNC_EXCEPTION_NAME(InvalidDataException);
}

QString InvalidDataException::qWhat() const
{
	return Exception::qWhat() +
			QStringLiteral("\n\tTypeName: %1")
			.arg(QString::fromUtf8(_typeName));
}

void InvalidDataException::raise() const
{
	throw (*this);
}

QException *InvalidDataException::clone() const
{
	return new InvalidDataException(this);
}
