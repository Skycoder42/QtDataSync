#include "exception.h"
#include "storageengine.h"
#include <QThread>
using namespace QtDataSync;

StorageEngine::StorageEngine(QJsonSerializer *serializer, LocalStore *localStore, StateHolder *stateHolder, RemoteConnector *remoteConnector, DataMerger *dataMerger) :
	QObject(),
	serializer(serializer),
	localStore(localStore),
	stateHolder(stateHolder),
	remoteConnector(remoteConnector),
	dataMerger(dataMerger),
	requestCache(),
	requestCounter(0)
{
	serializer->setParent(this);
	localStore->setParent(this);
	stateHolder->setParent(this);
	remoteConnector->setParent(this);
	dataMerger->setParent(this);
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
		case QtDataSync::StorageEngine::Count:
			count(futureInterface, metaTypeId);
			break;
		case QtDataSync::StorageEngine::LoadAll:
			loadAll(futureInterface, metaTypeId, value.toInt());
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

	stateHolder->initialize();
	remoteConnector->initialize();
	dataMerger->initialize();
}

void StorageEngine::finalize()
{
	dataMerger->finalize();
	remoteConnector->finalize();
	stateHolder->finalize();
	localStore->finalize();
	thread()->quit();
}

void StorageEngine::requestCompleted(quint64 id, const QJsonValue &result)
{
	auto info = requestCache.take(id);
	if(!result.isUndefined()) {
		try {
			auto obj = serializer->deserialize(result, info.metaTypeId);
			info.futureInterface.reportResult(obj);
		} catch(SerializerException &e) {
			info.futureInterface.reportException(Exception(e.qWhat()));
		}
	}

	if(info.changeAction) {
		if(info.changeKey.isEmpty()) {
			stateHolder->markAllLocalChanged(QMetaType::typeName(info.metaTypeId),
											 info.changeState);
		} else {
			stateHolder->markLocalChanged({QMetaType::typeName(info.metaTypeId), info.changeKey},
										  info.changeState);
		}
	}

	info.futureInterface.reportFinished();
}

void StorageEngine::requestFailed(quint64 id, const QString &errorString)
{
	auto info = requestCache.take(id);
	info.futureInterface.reportException(Exception(errorString));
	info.futureInterface.reportFinished();
}

void StorageEngine::count(QFutureInterface<QVariant> futureInterface, int metaTypeId)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, QMetaType::Int});
	localStore->count(id, QMetaType::typeName(metaTypeId));
}

void StorageEngine::loadAll(QFutureInterface<QVariant> futureInterface, int dataMetaTypeId, int listMetaTypeId)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, listMetaTypeId});
	localStore->loadAll(id, QMetaType::typeName(dataMetaTypeId));
}

void StorageEngine::load(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, const QString &value)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, metaTypeId});
	localStore->load(id, QMetaType::typeName(metaTypeId), key, value);
}

void StorageEngine::save(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, QVariant value)
{
	if(!value.convert(metaTypeId))
		throw Exception(QStringLiteral("Failed to convert value to %1").arg(QMetaType::typeName(metaTypeId)));

	try {
		auto json = serializer->serialize(value).toObject();
		auto keyValue = json[key].toVariant().toString();
		auto id = requestCounter++;
		RequestInfo info(futureInterface, metaTypeId);
		info.changeAction = true;
		info.changeKey = keyValue;
		info.changeState = StateHolder::Changed;
		requestCache.insert(id, info);
		localStore->save(id, QMetaType::typeName(metaTypeId), key, keyValue, json);
	} catch(SerializerException &e) {
		throw Exception(e.qWhat());
	}
}

void StorageEngine::remove(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, const QString &value)
{
	auto id = requestCounter++;
	RequestInfo info(futureInterface, metaTypeId);
	info.changeAction = true;
	info.changeKey = value;
	info.changeState = StateHolder::Deleted;
	requestCache.insert(id, info);
	localStore->remove(id, QMetaType::typeName(metaTypeId), key, value);
}

void StorageEngine::removeAll(QFutureInterface<QVariant> futureInterface, int metaTypeId)
{
	auto id = requestCounter++;
	RequestInfo info(futureInterface, metaTypeId);
	info.changeAction = true;
	info.changeState = StateHolder::Deleted;
	requestCache.insert(id, info);
	localStore->removeAll(id, QMetaType::typeName(metaTypeId));
}



StorageEngine::RequestInfo::RequestInfo(QFutureInterface<QVariant> futureInterface, int metaTypeId) :
	futureInterface(futureInterface),
	metaTypeId(metaTypeId),
	changeAction(false),
	changeKey(),
	changeState(StateHolder::Unchanged)
{}
