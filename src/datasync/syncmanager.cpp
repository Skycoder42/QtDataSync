#include "syncmanager.h"
#include "rep_syncmanager_p_replica.h"
#include "remoteconnector_p.h"
#include "setup_p.h"
#include "exchangeengine_p.h"
#include "defaults_p.h"

#include <tuple>

// ------------- Private classes Definition -------------

namespace QtDataSync {

class SyncManagerPrivateHolder
{
public:
	SyncManagerPrivateHolder(SyncManagerPrivateReplica *replica);

	SyncManagerPrivateReplica *replica;
	QHash<QUuid, std::function<void(SyncManager::SyncState)>> syncActions;
	QHash<QUuid, std::tuple<bool, bool>> initActions;
};

}

// ------------- Implementation -------------

using namespace QtDataSync;

SyncManager::SyncManager(QObject *parent) :
	SyncManager(DefaultSetup, parent)
{}

SyncManager::SyncManager(const QString &setupName, QObject *parent) :
	SyncManager(Defaults(DefaultsPrivate::obtainDefaults(setupName)).remoteNode(), parent)
{}

SyncManager::SyncManager(QRemoteObjectNode *node, QObject *parent) :
	QObject(parent),
	d(new SyncManagerPrivateHolder(node->acquire<SyncManagerPrivateReplica>()))
{
	d->replica->setParent(this);
	connect(d->replica, &SyncManagerPrivateReplica::syncEnabledChanged,
			this, &SyncManager::syncEnabledChanged);
	connect(d->replica, &SyncManagerPrivateReplica::syncStateChanged,
			this, &SyncManager::syncStateChanged);
	connect(d->replica, &SyncManagerPrivateReplica::syncProgressChanged,
			this, &SyncManager::syncProgressChanged);
	connect(d->replica, &SyncManagerPrivateReplica::lastErrorChanged,
			this, &SyncManager::lastErrorChanged);
	connect(d->replica, &SyncManagerPrivateReplica::stateReached,
			this, &SyncManager::onStateReached);
	connect(d->replica, &SyncManagerPrivateReplica::initialized,
			this, &SyncManager::onInit);
}

SyncManager::~SyncManager() {}

QRemoteObjectReplica *SyncManager::replica() const
{
	return d->replica;
}

bool SyncManager::isSyncEnabled() const
{
	return d->replica->syncEnabled();
}

SyncManager::SyncState SyncManager::syncState() const
{
	return d->replica->syncState();
}

qreal SyncManager::syncProgress() const
{
	return d->replica->syncProgress();
}

QString SyncManager::lastError() const
{
	return d->replica->lastError();
}

void SyncManager::setSyncEnabled(bool syncEnabled)
{
	d->replica->pushSyncEnabled(syncEnabled);
}

void SyncManager::synchronize()
{
	d->replica->synchronize();
}

void SyncManager::reconnect()
{
	d->replica->reconnect();
}

void SyncManager::runOnDownloaded(const std::function<void (SyncManager::SyncState)> &resultFn, bool triggerSync)
{
	runImp(true, triggerSync, resultFn);
}

void SyncManager::runOnSynchronized(const std::function<void (SyncManager::SyncState)> &resultFn, bool triggerSync)
{
	runImp(false, triggerSync, resultFn);
}

void SyncManager::onInit()
{
	for(auto it = d->initActions.constBegin(); it != d->initActions.constEnd(); it++)
		d->replica->runOnState(it.key(), std::get<0>(*it), std::get<1>(*it));
	d->initActions.clear();
}

void SyncManager::onStateReached(const QUuid &id, SyncManager::SyncState state)
{
	auto fn = d->syncActions.take(id);
	if(fn)
		fn(state);
}

void SyncManager::runImp(bool downloadOnly, bool triggerSync, const std::function<void (SyncManager::SyncState)> &resultFn)
{
	Q_ASSERT_X(resultFn, Q_FUNC_INFO, "runOn resultFn must be a valid function");
	auto id = QUuid::createUuid();
	d->syncActions.insert(id, resultFn);
	if(d->replica->isInitialized())
		d->replica->runOnState(id, downloadOnly, triggerSync);
	else
		d->initActions.insert(id, std::make_tuple(downloadOnly, triggerSync));
}



SyncManagerPrivateHolder::SyncManagerPrivateHolder(SyncManagerPrivateReplica *replica) :
	replica(replica),
	syncActions(),
	initActions()
{}



QDataStream &QtDataSync::operator<<(QDataStream &stream, const SyncManager::SyncState &state)
{
	stream << static_cast<int>(state);
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, SyncManager::SyncState &state)
{
	stream.startTransaction();
	stream >> reinterpret_cast<int&>(state);
	if(QMetaEnum::fromType<SyncManager::SyncState>().valueToKey(state)) //verify is a valid enum value
		stream.commitTransaction();
	else
		stream.abortTransaction();;
	return stream;
}
