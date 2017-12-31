#include "syncmanager.h"
#include "rep_syncmanager_p_replica.h"
#include "remoteconnector_p.h"
#include "setup_p.h"
#include "exchangeengine_p.h"
#include "defaults_p.h"
using namespace QtDataSync;

SyncManager::SyncManager(QObject *parent) :
	SyncManager(DefaultSetup, parent)
{}

SyncManager::SyncManager(const QString &setupName, QObject *parent) :
	SyncManager(Defaults(DefaultsPrivate::obtainDefaults(setupName)).remoteNode(), parent)
{}

SyncManager::SyncManager(QRemoteObjectNode *node, QObject *parent) :
	QObject(parent),
	d(node->acquire<SyncManagerPrivateReplica>())
{
	d->setParent(this);
	connect(d, &SyncManagerPrivateReplica::syncEnabledChanged,
			this, &SyncManager::syncEnabledChanged);
	connect(d, &SyncManagerPrivateReplica::syncStateChanged,
			this, &SyncManager::syncStateChanged);
	connect(d, &SyncManagerPrivateReplica::lastErrorChanged,
			this, &SyncManager::lastErrorChanged);
}

QRemoteObjectReplica *SyncManager::replica() const
{
	return d;
}

bool SyncManager::isSyncEnabled() const
{
	return d->syncEnabled();
}

SyncManager::SyncState SyncManager::syncState() const
{
	return d->syncState();
}

QString SyncManager::lastError() const
{
	return d->lastError();
}

void SyncManager::setSyncEnabled(bool syncEnabled)
{
	d->pushSyncEnabled(syncEnabled);
}

void SyncManager::synchronize()
{
	d->synchronize();
}

void SyncManager::reconnect()
{
	d->reconnect();
}

void SyncManager::runOnDownloaded(const std::function<void (SyncManager::SyncState)> &resultFn, bool triggerSync)
{
	runImp(true, triggerSync, resultFn);
}

void SyncManager::runOnSynchronized(const std::function<void (SyncManager::SyncState)> &resultFn, bool triggerSync)
{
	runImp(false, triggerSync, resultFn);
}

void SyncManager::runImp(bool downloadOnly, bool triggerSync, const std::function<void (SyncManager::SyncState)> &resultFn)
{
	auto state = d->syncState();

	switch (state) {
	case Error: //wont sync -> simply complete
	case Disconnected:
		resultFn(state);
		break;
	case Uploading: //if download only -> done
		if(downloadOnly) {
			resultFn(state);
			break;
		}
		Q_FALLTHROUGH();
	case Synchronized: //if wants sync -> trigger it, then...
		if(triggerSync)
			d->synchronize();
		Q_FALLTHROUGH();
	case Initializing: //conntect to react to result
	case Downloading:
	{
		auto resObj = new QObject(this);
		connect(d, &SyncManagerPrivateReplica::syncStateChanged, resObj, [resObj, resultFn, downloadOnly](SyncState state) {
			switch (state) {
			case Initializing: // do nothing
			case Downloading:
				break;
			case Uploading: // download only -> done, else do nothing
				if(!downloadOnly)
					break;
				Q_FALLTHROUGH();
			case Synchronized: //done
			case Error:
			case Disconnected:
				resultFn(state);
				resObj->deleteLater();
				break;
			default:
				Q_UNREACHABLE();
				break;
			}
		});
		break;
	}
	default:
		Q_UNREACHABLE();
		break;
	}
}



QDataStream &QtDataSync::operator<<(QDataStream &stream, const SyncManager::SyncState &state)
{
	stream << (int)state;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, SyncManager::SyncState &state)
{
	stream.startTransaction();
	stream >> (int&)state;
	stream.commitTransaction();
	return stream;
}
