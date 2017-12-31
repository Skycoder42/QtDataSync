#include "syncmanager_p.h"
using namespace QtDataSync;

SyncManagerPrivate::SyncManagerPrivate(ExchangeEngine *engineParent) :
	SyncManagerPrivateSource(engineParent),
	_engine(engineParent)
{
	connect(_engine, &ExchangeEngine::stateChanged,
			this, &SyncManagerPrivate::syncStateChanged);
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
