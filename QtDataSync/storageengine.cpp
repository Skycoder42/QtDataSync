#include "exception.h"
#include "storageengine.h"
#include <QThread>
using namespace QtDataSync;

StorageEngine::StorageEngine(QJsonSerializer *serializer, LocalStore *localStore) :
	QObject(),
	serializer(serializer),
	localStore(localStore),
	requestCache(),
	requestCounter(0)
{
	serializer->setParent(this);
	localStore->setParent(this);
}

void StorageEngine::beginTask(QFutureInterface<QVariant> futureInterface, StorageEngine::TaskType taskType, int metaTypeId, const QVariant &value)
{
	try {
		auto flags = QMetaType::typeFlags(metaTypeId);
		if(!flags.testFlag(QMetaType::PointerToQObject) &&
		   !flags.testFlag(QMetaType::IsGadget))
			throw Exception("You can only store QObjects or Q_GADGETs with QtDataSync!");

		auto metaObject = QMetaType::metaObjectForType(metaTypeId);
		auto userProp = metaObject->userProperty();

		switch (taskType) {
		case QtDataSync::StorageEngine::LoadAll:
			loadAll(futureInterface, metaTypeId);
			break;
		case QtDataSync::StorageEngine::Load:
			load(futureInterface, metaTypeId, userProp.name(), value.toString());
			break;
		case QtDataSync::StorageEngine::Save:
			save(futureInterface, metaTypeId, userProp.name(), value);
			break;
		case QtDataSync::StorageEngine::Remove:
			remove(futureInterface, metaTypeId, userProp.name(), value.toString());
			break;
		case QtDataSync::StorageEngine::RemoveAll:
			removeAll(futureInterface, metaTypeId);
			break;
		default:
			break;
		}
	} catch(QException &e) {
		futureInterface.reportException(e);
		futureInterface.reportFinished();
	}
}

void StorageEngine::initialize()
{
	connect(localStore, &LocalStore::requestCompleted,
			this, &StorageEngine::requestCompleted);
	connect(localStore, &LocalStore::requestFailed,
			this, &StorageEngine::requestFailed);
	localStore->initialize();
}

void StorageEngine::finalize()
{
	localStore->finalize();
	thread()->quit();
}

void StorageEngine::requestCompleted(quint64 id, const QJsonValue &result)
{
	auto info = requestCache.take(id);
	if(!result.isUndefined()) {
		try {
			auto obj = serializer->deserialize(result, info.second);
			qDebug() << "Loaded data:"
					 << obj;

			info.first.reportResult(obj);
		} catch(SerializerException &e) {
			info.first.reportException(Exception(e.qWhat()));
		}
	}

	info.first.reportFinished();
}

void StorageEngine::requestFailed(quint64 id, const QString &errorString)
{
	auto info = requestCache.take(id);
	info.first.reportException(Exception(errorString));
	info.first.reportFinished();
}

void StorageEngine::loadAll(QFutureInterface<QVariant> futureInterface, int metaTypeId)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, metaTypeId});
	localStore->loadAll(id, metaTypeId);
}

void StorageEngine::load(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, const QString &value)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, metaTypeId});
	localStore->load(id, metaTypeId, key, value);
}

void StorageEngine::save(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, QVariant value)
{
	if(!value.convert(metaTypeId))
		throw Exception(QStringLiteral("Failed to convert value to %1").arg(QMetaType::typeName(metaTypeId)));

	try {
		auto json = serializer->serialize(value);
		qDebug() << "Save for"
				 << key
				 << "of:"
				 << json;

		auto id = requestCounter++;
		requestCache.insert(id, {futureInterface, metaTypeId});
		localStore->save(id, metaTypeId, key, json.toObject());
	} catch(SerializerException &e) {
		throw Exception(e.qWhat());
	}
}

void StorageEngine::remove(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, const QString &value)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, metaTypeId});
	localStore->remove(id, metaTypeId, key, value);
}

void StorageEngine::removeAll(QFutureInterface<QVariant> futureInterface, int metaTypeId)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, metaTypeId});
	localStore->removeAll(id, metaTypeId);
}
