#include "syncmanager_p.h"
using namespace QtDataSync;

SyncManagerPrivate::SyncManagerPrivate(ExchangeEngine *engineParent) :
	SyncManagerPrivateSource(engineParent),
	_engine(engineParent)
{
	connect(_engine, &ExchangeEngine::stateChanged,
			this, &SyncManagerPrivate::syncStateChanged);
	connect(_engine, &ExchangeEngine::progressChanged,
			this, &SyncManagerPrivate::syncProgressChanged);
	connect(_engine, &ExchangeEngine::lastErrorChanged,
			this, &SyncManagerPrivate::lastErrorChanged);
	connect(_engine->remoteConnector(), &RemoteConnector::syncEnabledChanged,
			this, &SyncManagerPrivate::syncEnabledChanged);
}

bool SyncManagerPrivate::syncEnabled() const
{
	return _engine->remoteConnector()->isSyncEnabled();
}

SyncManager::SyncState SyncManagerPrivate::syncState() const
{
	return _engine->state();
}

qreal SyncManagerPrivate::syncProgress() const
{
	return _engine->progress();
}

QString SyncManagerPrivate::lastError() const
{
	return _engine->lastError();
}

void SyncManagerPrivate::setSyncEnabled(bool syncEnabled)
{
	_engine->remoteConnector()->setSyncEnabled(syncEnabled);
}

void SyncManagerPrivate::synchronize()
{
	_engine->remoteConnector()->resync();
}

void SyncManagerPrivate::reconnect()
{
	_engine->remoteConnector()->reconnect();
}

void SyncManagerPrivate::runOnState(const QUuid &id, bool downloadOnly, bool triggerSync)
{
	auto state = syncState();
	auto skipDOnly = false;

	switch(state) {
	case SyncManager::Error: //wont sync -> simply complete
	case SyncManager::Disconnected:
		emit stateReached(id, state);
		break;
	case SyncManager::Synchronized: //if wants sync -> trigger it, then...
		if(triggerSync) {
			synchronize();
			skipDOnly = true; //fallthrough in the uploading state
		} else {
			emit stateReached(id, state);
			break;
		}
		Q_FALLTHROUGH();
	case SyncManager::Uploading: //if download only -> done
		if(downloadOnly && !skipDOnly) {
			emit stateReached(id, state);
			break;
		}
		Q_FALLTHROUGH();
	case SyncManager::Initializing: //conntect to react to result
	case SyncManager::Downloading:
	{
		auto resObj = new QObject(this);
		connect(this, &SyncManagerPrivate::syncStateChanged, resObj, [this, resObj, id, downloadOnly](SyncManager::SyncState state) {
			switch (state) {
			case SyncManager::Initializing: //do nothing
			case SyncManager::Downloading:
				break;
			case SyncManager::Uploading: //download only -> done, else do nothing
				if(!downloadOnly)
					break;
				Q_FALLTHROUGH();
			case SyncManager::Synchronized: //done
			case SyncManager::Error:
			case SyncManager::Disconnected:
				emit stateReached(id, state);
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
