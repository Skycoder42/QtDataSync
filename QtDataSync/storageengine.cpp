#include "exception.h"
#include "storageengine.h"
#include "defaults.h"
#include <QThread>
using namespace QtDataSync;

#define LOG defaults->loggingCategory()

StorageEngine::StorageEngine(Defaults *defaults, QJsonSerializer *serializer, LocalStore *localStore, StateHolder *stateHolder, RemoteConnector *remoteConnector, DataMerger *dataMerger) :
	QObject(),
	defaults(defaults),
	serializer(serializer),
	localStore(localStore),
	stateHolder(stateHolder),
	remoteConnector(remoteConnector),
	changeController(new ChangeController(dataMerger, this)),
	requestCache(),
	requestCounter(0)
{
	defaults->setParent(this);
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
		case Count:
			count(futureInterface, metaTypeId);
			break;
		case Keys:
			keys(futureInterface, metaTypeId);
			break;
		case LoadAll:
			loadAll(futureInterface, metaTypeId, value.toInt());
			break;
		case Load:
			load(futureInterface, metaTypeId, userProp.name(), value.toString());
			break;
		case Save:
			save(futureInterface, metaTypeId, userProp.name(), value);
			break;
		case Remove:
			remove(futureInterface, metaTypeId, userProp.name(), value.toString());
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
			this, &StorageEngine::requestCompleted,
			Qt::QueuedConnection);
	connect(localStore, &LocalStore::requestFailed,
			this, &StorageEngine::requestFailed,
			Qt::QueuedConnection);

	//changeController
	connect(changeController, &ChangeController::loadLocalStatus,
			this, &StorageEngine::loadLocalStatus);
	connect(remoteConnector, &RemoteConnector::remoteStateChanged,
			changeController, &ChangeController::updateRemoteStatus);
	connect(changeController, &ChangeController::beginRemoteOperation,
			this, &StorageEngine::beginRemoteOperation,
			Qt::QueuedConnection);
	connect(changeController, &ChangeController::beginLocalOperation,
			this, &StorageEngine::beginLocalOperation,
			Qt::QueuedConnection);

	//remoteConnector
	connect(remoteConnector, &RemoteConnector::operationDone,
			this, &StorageEngine::operationDone,
			Qt::QueuedConnection);
	connect(remoteConnector, &RemoteConnector::operationFailed,
			this, &StorageEngine::operationFailed,
			Qt::QueuedConnection);

	localStore->initialize(defaults);
	stateHolder->initialize(defaults);
	changeController->initialize(defaults);
	remoteConnector->initialize(defaults);
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
		} else
			info.futureInterface.reportResult(QVariant());

		info.futureInterface.reportFinished();
	}

	if(info.changeAction) {
		stateHolder->markLocalChanged(info.changeKey, info.changeState);
		changeController->updateLocalStatus(info.changeKey, info.changeState);
	}
}

void StorageEngine::requestFailed(quint64 id, const QString &errorString)
{
	auto info = requestCache.take(id);
	if(info.isChangeControllerRequest) {
		qCCritical(LOG) << "Local operation failed with error:"
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
	qCWarning(LOG) << "Network operation failed with error:"
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
		qCWarning(LOG) << "Remote operation for invalid type"
					   << operation.key.first
					   << "requested!";
		return;
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
		qCWarning(LOG) << "Local operation for invalid type"
					   << operation.key.first
					   << "requested!";
		return;
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
		info.changeState = StateHolder::Unchanged;
		requestCache.insert(id, info);
		localStore->save(id, operation.key, operation.writeObject, userProperty.name());
		break;
	case ChangeController::Remove:
		info.changeAction = true;
		info.changeKey = operation.key;
		info.changeState = StateHolder::Unchanged;
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

void StorageEngine::keys(QFutureInterface<QVariant> futureInterface, int metaTypeId)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, QMetaType::QStringList});
	localStore->keys(id, QMetaType::typeName(metaTypeId));
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



StorageEngine::RequestInfo::RequestInfo(bool isChangeControllerRequest) :
	isChangeControllerRequest(isChangeControllerRequest),
	futureInterface(),
	convertMetaTypeId(QMetaType::UnknownType),
	changeAction(false),
	changeKey(),
	changeState(StateHolder::Unchanged)
{}

StorageEngine::RequestInfo::RequestInfo(QFutureInterface<QVariant> futureInterface, int convertMetaTypeId) :
	isChangeControllerRequest(false),
	futureInterface(futureInterface),
	convertMetaTypeId(convertMetaTypeId),
	changeAction(false),
	changeKey(),
	changeState(StateHolder::Unchanged)
{}
