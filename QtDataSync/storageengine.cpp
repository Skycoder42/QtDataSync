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
	changeController(new ChangeController(dataMerger, this)),
	requestCache(),
	requestCounter(0)
{
	serializer->setParent(this);
	localStore->setParent(this);
	stateHolder->setParent(this);
	remoteConnector->setParent(this);
}

void StorageEngine::beginTask(QFutureInterface<QVariant> futureInterface, StorageEngine::TaskType taskType, int metaTypeId, const QVariant &value)
{
	try {
		auto flags = QMetaType::typeFlags(metaTypeId);
		auto metaObject = QMetaType::metaObjectForType(metaTypeId);
		if((!flags.testFlag(QMetaType::PointerToQObject) &&
			!flags.testFlag(QMetaType::IsGadget)) ||
		   !metaObject)
			throw Exception("You can only store QObjects or Q_GADGETs with QtDataSync!");

		auto userProp = metaObject->userProperty();
		if(!userProp.isValid())
			throw Exception(QStringLiteral("To save a datatype, it requires a user property"));

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
	//localStore
	connect(localStore, &LocalStore::requestCompleted,
			this, &StorageEngine::requestCompleted);
	connect(localStore, &LocalStore::requestFailed,
			this, &StorageEngine::requestFailed);

	//changeController
	connect(changeController, &ChangeController::loadLocalStatus,
			this, &StorageEngine::loadLocalStatus);
	connect(remoteConnector, &RemoteConnector::remoteStateChanged,
			changeController, &ChangeController::updateRemoteStatus);
	connect(changeController, &ChangeController::beginRemoteOperation,
			this, &StorageEngine::beginRemoteOperation);
	connect(changeController, &ChangeController::beginLocalOperation,
			this, &StorageEngine::beginLocalOperation);

	//remoteConnector
	connect(remoteConnector, &RemoteConnector::operationDone,
			this, &StorageEngine::operationDone);
	connect(remoteConnector, &RemoteConnector::operationFailed,
			this, &StorageEngine::operationFailed);

	localStore->initialize();
	stateHolder->initialize();
	changeController->initialize();
	remoteConnector->initialize();
}

void StorageEngine::finalize()
{
	remoteConnector->finalize();
	changeController->finalize();
	stateHolder->finalize();
	localStore->finalize();
	thread()->quit();
}

void StorageEngine::requestCompleted(quint64 id, const QJsonValue &result)
{
	auto info = requestCache.take(id);

	if(info.isChangeControllerRequest)
		changeController->nextStage(true, result);
	else {
		if(!result.isUndefined()) {
			try {
				auto obj = serializer->deserialize(result, info.convertMetaTypeId);
				info.futureInterface.reportResult(obj);
			} catch(SerializerException &e) {
				info.futureInterface.reportException(Exception(e.qWhat()));
			}
		}

		info.futureInterface.reportFinished();
	}

	if(info.changeAction) {
		if(info.changeKey.second.isEmpty()) {
			stateHolder->markAllLocalChanged(info.changeKey.first,
											 info.changeState);
			loadLocalStatus();
		} else {
			stateHolder->markLocalChanged(info.changeKey, info.changeState);
			changeController->updateLocalStatus(info.changeKey, info.changeState);
		}
	}
}

void StorageEngine::requestFailed(quint64 id, const QString &errorString)
{
	auto info = requestCache.take(id);
	if(info.isChangeControllerRequest) {
		qWarning() << "Local operation failed with error:"
				   << errorString;
		changeController->nextStage(false);
	} else {
		info.futureInterface.reportException(Exception(errorString));
		info.futureInterface.reportFinished();
	}
}

void StorageEngine::operationDone(const QJsonValue &result)
{
	changeController->nextStage(true, result);
}

void StorageEngine::operationFailed(const QString &errorString)
{
	qWarning() << "Network operation failed with error:"
			   << errorString;
	changeController->nextStage(false);
}

void StorageEngine::loadLocalStatus()
{
	auto state = stateHolder->listLocalChanges();
	changeController->setInitialLocalStatus(state);
}

void StorageEngine::beginRemoteOperation(const ChangeController::ChangeOperation &operation)
{
	auto metaTypeId = QMetaType::type(operation.key.first);
	auto metaObject = QMetaType::metaObjectForType(metaTypeId);
	if(!metaObject) {
		qWarning() << "Remote operation for invalid type"
				   << operation.key.first
				   << "requested!";
	}

	auto userProperty = QMetaType::metaObjectForType(metaTypeId)->userProperty();
	switch (operation.operation) {
	case ChangeController::Load:
		remoteConnector->download(operation.key, userProperty.name());
		break;
	case ChangeController::Save:
		remoteConnector->upload(operation.key, operation.writeObject, userProperty.name());
		break;
	case ChangeController::Remove:
		remoteConnector->remove(operation.key, userProperty.name());
		break;
	case ChangeController::MarkUnchanged:
		remoteConnector->markUnchanged(operation.key, userProperty.name());
		break;
	default:
		Q_UNREACHABLE();
	}
}

void StorageEngine::beginLocalOperation(const ChangeController::ChangeOperation &operation)
{
	auto metaTypeId = QMetaType::type(operation.key.first);
	auto metaObject = QMetaType::metaObjectForType(metaTypeId);
	if(!metaObject) {
		qWarning() << "Remote operation for invalid type"
				   << operation.key.first
				   << "requested!";
	}

	auto id = requestCounter++;
	RequestInfo info(true);

	auto userProperty = metaObject->userProperty();
	switch (operation.operation) {
	case ChangeController::Load:
		requestCache.insert(id, info);
		localStore->load(id, operation.key, userProperty.name());
		break;
	case ChangeController::Save:
		info.changeAction = true;
		info.changeKey = operation.key;
		info.changeState = StateHolder::Changed;
		requestCache.insert(id, info);
		localStore->save(id, operation.key, operation.writeObject, userProperty.name());
		break;
	case ChangeController::Remove:
		requestCache.insert(id, info);
		localStore->remove(id, operation.key, userProperty.name());
		break;
	case ChangeController::MarkUnchanged:
		stateHolder->markLocalChanged(operation.key, StateHolder::Unchanged);
		changeController->nextStage(true);
		break;
	default:
		Q_UNREACHABLE();
	}
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

void StorageEngine::load(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QByteArray &keyProperty, const QString &value)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, metaTypeId});
	localStore->load(id, {QMetaType::typeName(metaTypeId), value}, keyProperty);
}

void StorageEngine::save(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QByteArray &keyProperty, QVariant value)
{
	if(!value.convert(metaTypeId))
		throw Exception(QStringLiteral("Failed to convert value to %1").arg(QMetaType::typeName(metaTypeId)));

	try {
		auto json = serializer->serialize(value).toObject();
		auto id = requestCounter++;
		RequestInfo info(futureInterface, metaTypeId);
		info.changeAction = true;
		info.changeKey = {QMetaType::typeName(metaTypeId), json[keyProperty].toVariant().toString()};
		info.changeState = StateHolder::Changed;
		requestCache.insert(id, info);
		localStore->save(id, info.changeKey, json, keyProperty);
	} catch(SerializerException &e) {
		throw Exception(e.qWhat());
	}
}

void StorageEngine::remove(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QByteArray &keyProperty, const QString &value)
{
	auto id = requestCounter++;
	RequestInfo info(futureInterface, metaTypeId);
	info.changeAction = true;
	info.changeKey = {QMetaType::typeName(metaTypeId), value};
	info.changeState = StateHolder::Deleted;
	requestCache.insert(id, info);
	localStore->remove(id, info.changeKey, keyProperty);
}

void StorageEngine::removeAll(QFutureInterface<QVariant> futureInterface, int metaTypeId)
{
	auto id = requestCounter++;
	RequestInfo info(futureInterface, metaTypeId);
	info.changeAction = true;
	info.changeKey = {QMetaType::typeName(metaTypeId), {}};
	info.changeState = StateHolder::Deleted;
	requestCache.insert(id, info);
	localStore->removeAll(id, QMetaType::typeName(metaTypeId));
}



StorageEngine::RequestInfo::RequestInfo(bool isChangeControllerRequest) :
	isChangeControllerRequest(isChangeControllerRequest),
	futureInterface(),
	convertMetaTypeId(QMetaType::UnknownType),
	changeAction(false),
	changeKey(),
	changeState(StateHolder::Unchanged)
{}

StorageEngine::RequestInfo::RequestInfo(QFutureInterface<QVariant> futureInterface, int metaTypeId) :
	isChangeControllerRequest(false),
	futureInterface(futureInterface),
	convertMetaTypeId(metaTypeId),
	changeAction(false),
	changeKey(),
	changeState(StateHolder::Unchanged)
{}
