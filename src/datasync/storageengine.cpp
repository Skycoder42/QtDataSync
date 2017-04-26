#include "exceptions.h"
#include "storageengine_p.h"
#include "defaults.h"

#include <QtCore/QThread>
#include <QtCore/QDateTime>

using namespace QtDataSync;

#define LOG defaults->loggingCategory()

StorageEngine::StorageEngine(Defaults *defaults, QJsonSerializer *serializer, LocalStore *localStore, StateHolder *stateHolder, RemoteConnector *remoteConnector, DataMerger *dataMerger, Encryptor *encryptor) :
	QObject(),
	defaults(defaults),
	serializer(serializer),
	localStore(localStore),
	stateHolder(stateHolder),
	remoteConnector(remoteConnector),
	encryptor(encryptor),
	changeController(new ChangeController(dataMerger, this)),
	requestCache(),
	requestCounter(0),
	controllerLock(QReadWriteLock::Recursive),
	currentSyncState(SyncController::Loading),
	currentAuthError()
{
	defaults->setParent(this);
	serializer->setParent(this);
	localStore->setParent(this);
	stateHolder->setParent(this);
	remoteConnector->setParent(this);
	if(encryptor)
		encryptor->setParent(this);
}

bool StorageEngine::isSyncEnabled() const
{
	QReadLocker _(&controllerLock);
	return remoteConnector->isSyncEnabled();
}

SyncController::SyncState StorageEngine::syncState() const
{
	QReadLocker _(&controllerLock);
	return currentSyncState;
}

QString StorageEngine::authenticationError() const
{
	QReadLocker _(&controllerLock);
	return currentAuthError;
}

void StorageEngine::beginTask(QFutureInterface<QVariant> futureInterface, QThread *targetThread, StorageEngine::TaskType taskType, int metaTypeId, const QVariant &value)
{
	try {
		auto flags = QMetaType::typeFlags(metaTypeId);
		auto metaObject = QMetaType::metaObjectForType(metaTypeId);
		if((!flags.testFlag(QMetaType::PointerToQObject) &&
			!flags.testFlag(QMetaType::IsGadget)) ||
		   !metaObject)
			throw DataSyncException("You can only store QObjects or Q_GADGETs with QtDataSync!");

		auto userProp = metaObject->userProperty();
		if(!userProp.isValid())
			throw DataSyncException("To store a datatype, it requires a user property");

		switch (taskType) {
		case Count:
			count(futureInterface, targetThread, metaTypeId);
			break;
		case Keys:
			keys(futureInterface, targetThread, metaTypeId);
			break;
		case LoadAll:
			loadAll(futureInterface, targetThread, metaTypeId, value.toInt());
			break;
		case Load:
			load(futureInterface, targetThread, metaTypeId, userProp.name(), value.toString());
			break;
		case Save:
			save(futureInterface, targetThread, metaTypeId, userProp.name(), value);
			break;
		case Remove:
			remove(futureInterface, targetThread, metaTypeId, userProp.name(), value.toString());
			break;
		case Search:
			search(futureInterface, targetThread, metaTypeId, value.value<QPair<int, QString>>());
			break;
		default:
			break;
		}
	} catch(QException &e) {
		futureInterface.reportException(e);
		futureInterface.reportFinished();
	}
}

void StorageEngine::triggerSync()
{
	remoteConnector->reloadRemoteState();
	loadLocalStatus();
}

void StorageEngine::triggerResync()
{
	remoteConnector->requestResync();
}

void StorageEngine::initialize()
{
	qsrand(QDateTime::currentMSecsSinceEpoch());

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
	connect(changeController, &ChangeController::updateSyncState,
			this, &StorageEngine::updateSyncState);
	connect(changeController, &ChangeController::updateSyncProgress,
			this, &StorageEngine::syncOperationsChanged);
	connect(remoteConnector, &RemoteConnector::remoteStateChanged,
			changeController, &ChangeController::setRemoteStatus);
	connect(remoteConnector, &RemoteConnector::remoteDataChanged,
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
	connect(remoteConnector, &RemoteConnector::authenticationFailed,
			this, &StorageEngine::authError,
			Qt::QueuedConnection);
	connect(remoteConnector, &RemoteConnector::clearAuthenticationError,
			this, &StorageEngine::clearAuthError,
			Qt::QueuedConnection);
	connect(remoteConnector, &RemoteConnector::performLocalReset,
			this, &StorageEngine::performLocalReset,
			Qt::DirectConnection);//explicitly direct connected -> blocking

	localStore->initialize(defaults);
	stateHolder->initialize(defaults);
	changeController->initialize(defaults);
	if(encryptor)
		encryptor->initialize(defaults);
	remoteConnector->initialize(defaults, encryptor);
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

	if(info.isChangeControllerRequest) {
		changeController->nextStage(true, result);
	} else {
		if(!result.isUndefined()) {
			try {
				auto obj = serializer->deserialize(result, info.convertMetaTypeId);
				if(info.targetThread)
					tryMoveToThread(obj, info.targetThread);
				info.futureInterface.reportResult(obj);
			} catch(QJsonSerializerException &e) {
				info.futureInterface.reportException(e);
			}
		} else
			info.futureInterface.reportResult(QVariant());

		info.futureInterface.reportFinished();
	}

	if(info.isDeleteAction && !result.toBool())
		return;

	if(info.changeAction) {
		stateHolder->markLocalChanged(info.changeKey, info.changeState);
		changeController->updateLocalStatus(info.changeKey, info.changeState);
	}

	if(!info.notifyKey.first.isNull())
		emit notifyChanged(QMetaType::type(info.notifyKey.first), info.notifyKey.second, info.isDeleteAction);
}

void StorageEngine::requestFailed(quint64 id, const QString &errorString)
{
	auto info = requestCache.take(id);
	if(info.isChangeControllerRequest) {
		qCCritical(LOG) << "Local operation failed with error:"
						<< errorString;
		changeController->nextStage(false);
	} else {
		info.futureInterface.reportException(DataSyncException(errorString));
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

void StorageEngine::authError(const QString &reason)
{
	QWriteLocker _(&controllerLock);
	currentAuthError = reason;
	emit authenticationErrorChanged(reason);
}

void StorageEngine::clearAuthError()
{
	QWriteLocker _(&controllerLock);
	if(!currentAuthError.isNull()) {
		currentAuthError.clear();
		emit authenticationErrorChanged({});
	}
}

void StorageEngine::loadLocalStatus()
{
	auto state = stateHolder->listLocalChanges();
	changeController->setInitialLocalStatus(state, true);
}

void StorageEngine::updateSyncState(SyncController::SyncState state)
{
	QWriteLocker _(&controllerLock);
	if(state != currentSyncState) {
		currentSyncState = state;
		emit syncStateChanged(state);
	}
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
		info.notifyKey = operation.key;
		info.isDeleteAction = false;
		info.changeAction = true;
		info.changeKey = operation.key;
		info.changeState = StateHolder::Unchanged;
		requestCache.insert(id, info);
		localStore->save(id, operation.key, operation.writeObject, userProperty.name());
		break;
	case ChangeController::Remove:
		info.notifyKey = operation.key;
		info.isDeleteAction = true;
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

void StorageEngine::performLocalReset(bool clearStore)
{
	if(clearStore) {
		localStore->resetStore();
		stateHolder->clearAllChanges();
		changeController->setInitialLocalStatus({}, false);
		emit notifyResetted();
	} else {
		auto state = stateHolder->resetAllChanges(localStore->loadAllKeys());
		changeController->setInitialLocalStatus(state, true);
	}
}

void StorageEngine::count(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int metaTypeId)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, targetThread, QMetaType::Int});
	localStore->count(id, QMetaType::typeName(metaTypeId));
}

void StorageEngine::keys(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int metaTypeId)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, targetThread, QMetaType::QStringList});
	localStore->keys(id, QMetaType::typeName(metaTypeId));
}

void StorageEngine::loadAll(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int dataMetaTypeId, int listMetaTypeId)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, targetThread, listMetaTypeId});
	localStore->loadAll(id, QMetaType::typeName(dataMetaTypeId));
}

void StorageEngine::load(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int metaTypeId, const QByteArray &keyProperty, const QString &value)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, targetThread, metaTypeId});
	localStore->load(id, {QMetaType::typeName(metaTypeId), value}, keyProperty);
}

void StorageEngine::save(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int metaTypeId, const QByteArray &keyProperty, QVariant value)
{
	if(!value.convert(metaTypeId)) {
		throw QJsonSerializationException(QStringLiteral("Failed to convert value to %1")
										  .arg(QString::fromUtf8(QMetaType::typeName(metaTypeId)))
										  .toUtf8());
	}

	auto json = serializer->serialize(value).toObject();
	auto id = requestCounter++;
	RequestInfo info(futureInterface, targetThread, metaTypeId);
	info.notifyKey = {QMetaType::typeName(metaTypeId), json[QString::fromUtf8(keyProperty)].toVariant().toString()};
	info.isDeleteAction = false;
	info.changeAction = true;
	info.changeKey = info.notifyKey;
	info.changeState = StateHolder::Changed;
	requestCache.insert(id, info);
	localStore->save(id, info.changeKey, json, keyProperty);
}

void StorageEngine::remove(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int metaTypeId, const QByteArray &keyProperty, const QString &value)
{
	auto id = requestCounter++;
	RequestInfo info(futureInterface, targetThread, QMetaType::Bool);
	info.notifyKey = {QMetaType::typeName(metaTypeId), value};
	info.isDeleteAction = true;
	info.changeAction = true;
	info.changeKey = info.notifyKey;
	info.changeState = StateHolder::Deleted;
	requestCache.insert(id, info);
	localStore->remove(id, info.changeKey, keyProperty);
}

void StorageEngine::search(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int dataMetaTypeId, QPair<int, QString> data)
{
	auto id = requestCounter++;
	requestCache.insert(id, {futureInterface, targetThread, data.first});
	localStore->search(id, QMetaType::typeName(dataMetaTypeId), data.second);
}

void StorageEngine::tryMoveToThread(QVariant object, QThread *thread) const
{
	if(object.canConvert(QVariant::List) && object.convert(QVariant::List)) {
		foreach(auto obj, object.toList())
			tryMoveToThread(obj, thread);
	} else {
		auto obj = object.value<QObject*>();
		if(obj)
			obj->moveToThread(thread);
	}
}



StorageEngine::RequestInfo::RequestInfo(bool isChangeControllerRequest) :
	isChangeControllerRequest(isChangeControllerRequest),
	futureInterface(),
	targetThread(nullptr),
	convertMetaTypeId(QMetaType::UnknownType),
	notifyKey(),
	isDeleteAction(false),
	changeAction(false),
	changeKey(),
	changeState(StateHolder::Unchanged)
{}

StorageEngine::RequestInfo::RequestInfo(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int convertMetaTypeId) :
	isChangeControllerRequest(false),
	futureInterface(futureInterface),
	targetThread(targetThread),
	convertMetaTypeId(convertMetaTypeId),
	notifyKey(),
	isDeleteAction(false),
	changeAction(false),
	changeKey(),
	changeState(StateHolder::Unchanged)
{}
