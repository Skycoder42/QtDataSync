#include "changecontroller.h"

#include <QDebug>
using namespace QtDataSync;

#define UNITE_STATE(x, y) (x | (y << 16))

ChangeController::ChangeController(DataMerger *merger, QObject *parent) :
	QObject(parent),
	merger(merger),
	localReady(false),
	remoteReady(false),
	localState(),
	remoteState(),
	currentMode(DoNothing),
	currentKey(),
	currentState(DoneState)
{
	merger->setParent(this);
}

void ChangeController::initialize()
{
	merger->initialize();
}

void ChangeController::finalize()
{
	merger->finalize();
}

void ChangeController::setInitialLocalStatus(const StateHolder::ChangeHash &changes)
{
	for(auto it = changes.constBegin(); it != changes.constEnd(); it++){
		qDebug() << "CHANGE?" << it.key() << it.value();
		localState.insert(it.key(), it.value());
	}
	localReady = true;
	newChanges();
}

void ChangeController::updateLocalStatus(const ObjectKey &key, StateHolder::ChangeState &state)
{
	qDebug() << "CHANGE!" << key << state;
	if(state == StateHolder::Unchanged)
		localState.remove(key);//unchange does not trigger sync
	else {
		localState.insert(key, state);
		newChanges();
	}
}

void ChangeController::updateRemoteStatus(bool canUpdate, const StateHolder::ChangeHash &changes)
{
	for(auto it = changes.constBegin(); it != changes.constEnd(); it++)
		remoteState.insert(it.key(), it.value());
	remoteReady = canUpdate;
	newChanges();
}

void ChangeController::nextStage(bool success, const QJsonValue &result)
{
	if(!success)//the value is skipped for now -> done by forcing "done" state
		currentState = DoneState;

	//in case of done --> go to the next one!
	if(currentState == DoneState)
		generateNextAction();

	switch (currentMode) {
	case DoNothing:
		// for now, all changes are synced!
		break;
	case DownloadRemote:
		actionDownloadRemote(result);
		break;
	case DeleteLocal:
		actionDeleteLocal();
		break;
	case UploadLocal:
		actionUploadLocal(result);
		break;
	case Merge:
		actionMerge(result);
		break;
	case DeleteRemote:
		actionDeleteRemote();
		break;
	case MarkAsUnchanged:
		actionMarkAsUnchanged();
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void ChangeController::newChanges()
{
	if(remoteReady && !localReady) {//load local on demand
		emit loadLocalStatus();
	} else if(remoteReady && localReady) {
		if(currentMode == DoNothing)//only if not already syncing!
			nextStage(false);//false to provoke state reset
	}
}

void ChangeController::generateNextAction()
{
	currentMode = DoNothing;

	do {
		currentKey = {};
		if(!remoteState.isEmpty())
			currentKey = remoteState.keys().first();
		else if(!localState.isEmpty())
			currentKey = localState.keys().first();
		else
			break;

		auto local = localState.take(currentKey);
		auto remote = remoteState.take(currentKey);

		switch (UNITE_STATE(local, remote)) {
		case UNITE_STATE(StateHolder::Unchanged, StateHolder::Unchanged)://u:u -> do nothing
			break;
		case UNITE_STATE(StateHolder::Unchanged, StateHolder::Changed)://u:c -> download
			currentMode = DownloadRemote;
			break;
		case UNITE_STATE(StateHolder::Unchanged, StateHolder::Deleted)://u:d -> delete local
			currentMode = DeleteLocal;
			break;
		case UNITE_STATE(StateHolder::Changed, StateHolder::Unchanged)://c:u -> upload
			currentMode = UploadLocal;
			break;
		case UNITE_STATE(StateHolder::Changed, StateHolder::Changed)://c:c -> MERGE
			switch (merger->mergePolicy()) {
			case DataMerger::KeepLocal://[c]:c -> upload
				currentMode = UploadLocal;
				break;
			case DataMerger::KeepRemote://c:[c] -> download
				currentMode = DownloadRemote;
				break;
			case DataMerger::Merge://[c]:[c] -> merge
				currentMode = Merge;
				break;
			default:
				Q_UNREACHABLE();
				break;
			}
		case UNITE_STATE(StateHolder::Changed, StateHolder::Deleted)://c:d -> POLICY
			switch (merger->syncPolicy()) {
			case QtDataSync::DataMerger::PreferLocal://[c]:d -> upload
			case QtDataSync::DataMerger::PreferUpdated:
				currentMode = UploadLocal;
				break;
			case QtDataSync::DataMerger::PreferRemote://c:[d] -> delete local
			case QtDataSync::DataMerger::PreferDeleted:
				currentMode = DeleteLocal;
				break;
			default:
				Q_UNREACHABLE();
				break;
			}
		case UNITE_STATE(StateHolder::Deleted, StateHolder::Unchanged)://d:u -> delete remote
			currentMode = DeleteRemote;
			break;
		case UNITE_STATE(StateHolder::Deleted, StateHolder::Changed)://d:c -> POLICY
			switch (merger->syncPolicy()) {
			case QtDataSync::DataMerger::PreferLocal://[d]:c -> delete remote
			case QtDataSync::DataMerger::PreferDeleted:
				currentMode = DeleteRemote;
				break;
			case QtDataSync::DataMerger::PreferRemote://d:[c] -> download
			case QtDataSync::DataMerger::PreferUpdated:
				currentMode = DownloadRemote;
				break;
			default:
				Q_UNREACHABLE();
				break;
			}
		case UNITE_STATE(StateHolder::Deleted, StateHolder::Deleted)://d:d -> mark unchanged
			currentMode = MarkAsUnchanged;
			break;
		default:
			Q_UNREACHABLE();
			break;
		}
	} while(currentMode == DoNothing);

	switch (currentMode) {
	case DoNothing:
		currentState = DoneState;
		break;
	case DownloadRemote:
		currentState = DownloadState;//download -> save[unchanged] -> remote unchanged
		break;
	case DeleteLocal:
		currentState = RemoveLocalState;//remove local[unchanged] -> remote unchanged
		break;
	case UploadLocal:
		currentState = LoadState;//load -> upload[unchanged] -> local unchanged
		break;
	case Merge:
		currentState = DownloadState;//download -> load -> [merge]save[unchanged] -> upload[unchanged]
		break;
	case DeleteRemote:
		currentState = RemoveRemoteState;//remove remote[unchanged] -> local unchanged
		break;
	case MarkAsUnchanged:
		currentState = LocalMarkState;//local unchanged -> remote unchanged
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void ChangeController::actionDownloadRemote(const QJsonValue &result)
{
	ChangeOperation operation;
	operation.key = currentKey;

	switch (currentState) {
	case DownloadState:
		operation.operation = Load;
		emit beginRemoteOperation(operation);
		currentState = SaveState;
		break;
	case SaveState:
		operation.operation = Save;
		operation.writeObject = result.toObject();
		emit beginLocalOperation(operation);
		currentState = RemoteMarkState;
		break;
	case RemoteMarkState:
		operation.operation = MarkUnchanged;
		emit beginRemoteOperation(operation);
		currentState = DoneState;
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void ChangeController::actionDeleteLocal()
{
	ChangeOperation operation;
	operation.key = currentKey;

	switch (currentState) {
	case RemoveLocalState:
		operation.operation = Remove;
		emit beginLocalOperation(operation);
		currentState = RemoteMarkState;
		break;
	case RemoteMarkState:
		operation.operation = MarkUnchanged;
		emit beginRemoteOperation(operation);
		currentState = DoneState;
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void ChangeController::actionUploadLocal(const QJsonValue &result)
{
	ChangeOperation operation;
	operation.key = currentKey;

	switch (currentState) {
	case LoadState:
		operation.operation = Load;
		emit beginLocalOperation(operation);
		currentState = UploadState;
		break;
	case UploadState:
		operation.operation = Save;
		operation.writeObject = result.toObject();
		emit beginRemoteOperation(operation);
		currentState = LocalMarkState;
		break;
	case LocalMarkState:
		operation.operation = MarkUnchanged;
		emit beginLocalOperation(operation);
		currentState = DoneState;
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void ChangeController::actionMerge(const QJsonValue &result)
{
	ChangeOperation operation;
	operation.key = currentKey;

	switch (currentState) {
	case DownloadState:
		operation.operation = Load;
		emit beginRemoteOperation(operation);
		currentState = LoadState;
		break;
	case LoadState:
		currentObject = result.toObject();//temp save the download result

		operation.operation = Load;
		emit beginLocalOperation(operation);
		currentState = SaveState;
		break;
	case SaveState:
		currentObject = merger->merge(result.toObject(), currentObject);//merge into temp var

		operation.operation = Save;
		operation.writeObject = currentObject;
		emit beginLocalOperation(operation);
		currentState = UploadState;
		break;
	case UploadState:
		operation.operation = Save;
		operation.writeObject = currentObject;
		emit beginRemoteOperation(operation);
		currentObject = {};
		currentState = DoneState;
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void ChangeController::actionDeleteRemote()
{
	ChangeOperation operation;
	operation.key = currentKey;

	switch (currentState) {
	case RemoveRemoteState:
		operation.operation = Remove;
		emit beginRemoteOperation(operation);
		currentState = LocalMarkState;
		break;
	case LocalMarkState:
		operation.operation = MarkUnchanged;
		emit beginLocalOperation(operation);
		currentState = DoneState;
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void ChangeController::actionMarkAsUnchanged()
{
	ChangeOperation operation;
	operation.key = currentKey;

	switch (currentState) {
	case LocalMarkState:
		operation.operation = MarkUnchanged;
		emit beginLocalOperation(operation);
		currentState = RemoteMarkState;
		break;
	case RemoteMarkState:
		operation.operation = MarkUnchanged;
		emit beginRemoteOperation(operation);
		currentState = DoneState;
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}
