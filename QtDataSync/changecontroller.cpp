#include "changecontroller.h"
using namespace QtDataSync;

ChangeController::ChangeController(DataMerger *merger, QObject *parent) :
	QObject(parent),
	merger(merger)
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
	for(auto it = changes.constBegin(); it != changes.constEnd(); it++)
		localState.insert(it.key(), it.value());
	localReady = true;
	reloadChangeList();
}

void ChangeController::updateLocalStatus(const ObjectKey &key, StateHolder::ChangeState &state)
{
	if(state == StateHolder::Unchanged) {
		if(localState.remove(key) > 0)
			reloadChangeList();
	} else {
		localState.insert(key, state);
		reloadChangeList();
	}
}

void ChangeController::updateRemoteStatus(bool canUpdate, const StateHolder::ChangeHash &changes)
{
	for(auto it = changes.constBegin(); it != changes.constEnd(); it++)
		remoteState.insert(it.key(), it.value());
	remoteReady = canUpdate;
	reloadChangeList();
}

void ChangeController::nextStage(bool success, const QJsonValue &result)
{

}

void ChangeController::reloadChangeList()
{
	if(remoteReady && !localReady)
		emit loadLocalStatus();
	else if(remoteReady && localReady) {

	} else {

	}
}
