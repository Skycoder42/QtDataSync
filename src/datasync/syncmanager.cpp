#include "syncmanager.h"
#include "rep_syncmanager_p_replica.h"
#include "remoteconnector_p.h"
#include "setup_p.h"
#include "exchangeengine_p.h"
#include "defaults_p.h"

#include "signal_private_connect_p.h"

#include <tuple>

// ------------- Private classes Definition -------------

namespace QtDataSync {

class SyncManagerPrivateHolder
{
public:
	SyncManagerPrivateHolder();

	SyncManagerPrivateReplica *replica;
	QHash<QUuid, std::function<void(SyncManager::SyncState)>> syncActions;
	QHash<QUuid, std::tuple<bool, bool>> initActions;
};

}

// ------------- Implementation -------------

using namespace QtDataSync;
using std::function;
using std::tuple;
using std::make_tuple;
using std::get;

SyncManager::SyncManager(QObject *parent) :
	SyncManager(DefaultSetup, parent)
{}

SyncManager::SyncManager(const QString &setupName, QObject *parent) :
	QObject(parent),
	d(new SyncManagerPrivateHolder())
{
	initReplica(setupName);
}

SyncManager::SyncManager(QRemoteObjectNode *node, QObject *parent) :
	QObject(parent),
	d(new SyncManagerPrivateHolder())
{
	initReplica(node);
}

SyncManager::SyncManager(QObject *parent, void *) :
	QObject(parent),
	d(new SyncManagerPrivateHolder())
{} //no init yet

void SyncManager::initReplica(const QString &setupName)
{
	initReplica(Defaults(DefaultsPrivate::obtainDefaults(setupName)).remoteNode());
}

void SyncManager::initReplica(QRemoteObjectNode *node)
{
	d->replica = node->acquire<SyncManagerPrivateReplica>();
	d->replica->setParent(this);
	connect(d->replica, &SyncManagerPrivateReplica::syncEnabledChanged,
			this, PSIG(&SyncManager::syncEnabledChanged));
	connect(d->replica, &SyncManagerPrivateReplica::syncStateChanged,
			this, PSIG(&SyncManager::syncStateChanged));
	connect(d->replica, &SyncManagerPrivateReplica::syncProgressChanged,
			this, PSIG(&SyncManager::syncProgressChanged));
	connect(d->replica, &SyncManagerPrivateReplica::lastErrorChanged,
			this, PSIG(&SyncManager::lastErrorChanged));
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

void SyncManager::runOnDownloaded(const function<void (SyncManager::SyncState)> &resultFn, bool triggerSync)
{
	runImp(true, triggerSync, resultFn);
}

void SyncManager::runOnSynchronized(const function<void (SyncManager::SyncState)> &resultFn, bool triggerSync)
{
	runImp(false, triggerSync, resultFn);
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

void SyncManager::onInit()
{
	for(auto it = d->initActions.constBegin(); it != d->initActions.constEnd(); it++)
		d->replica->runOnState(it.key(), get<0>(*it), get<1>(*it));
	d->initActions.clear();
}

void SyncManager::onStateReached(const QUuid &id, SyncManager::SyncState state)
{
	auto fn = d->syncActions.take(id);
	if(fn)
		fn(state);
}

void SyncManager::runImp(bool downloadOnly, bool triggerSync, const function<void (SyncManager::SyncState)> &resultFn)
{
	Q_ASSERT_X(resultFn, Q_FUNC_INFO, "runOn resultFn must be a valid function");
	auto id = QUuid::createUuid();
	d->syncActions.insert(id, resultFn);
	if(d->replica->isInitialized())
		d->replica->runOnState(id, downloadOnly, triggerSync);
	else
		d->initActions.insert(id, make_tuple(downloadOnly, triggerSync));
}



SyncManagerPrivateHolder::SyncManagerPrivateHolder() :
	replica(nullptr),
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
