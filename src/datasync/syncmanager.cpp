#include "syncmanager.h"
#include "syncmanager_p.h"
#include "remoteconnector_p.h"
#include "setup_p.h"
#include "exchangeengine_p.h"
using namespace QtDataSync;

SyncManager::SyncManager(QObject *parent, bool blockingConstruct) :
	SyncManager(DefaultSetup, parent, blockingConstruct)
{}

SyncManager::SyncManager(const QString &setupName, QObject *parent, bool blockingConstruct) :
	QObject(parent),
	d(new SyncManagerPrivate(setupName, this, blockingConstruct))
{}

SyncManager::~SyncManager() {}

bool SyncManager::isSyncEnabled() const
{
	return d->cEnabled;
}

SyncManager::SyncState SyncManager::syncState() const
{
	return d->cState;
}

QString SyncManager::lastError() const
{
	return d->cError;
}

void SyncManager::setSyncEnabled(bool syncEnabled)
{
	QMetaObject::invokeMethod(d->remoteConnector(), "setSyncEnabled",
							  Q_ARG(bool, syncEnabled));
	if(d->cEnabled != syncEnabled) {
		d->cEnabled = syncEnabled;
		emit syncEnabledChanged(syncEnabled);
	}
}

void SyncManager::synchronize()
{
	QMetaObject::invokeMethod(d->remoteConnector(), "resync");
}

void SyncManager::reconnect()
{
	QMetaObject::invokeMethod(d->remoteConnector(), "reconnect");
}

void SyncManager::runOnDownloaded(const std::function<void (SyncManager::SyncState)> &resultFn, bool triggerSync)
{
	QMetaObject::invokeMethod(d->engine(), "syncForResult",
							  Q_ARG(SyncResultObject*, new SyncResultObject(resultFn, this)),
							  Q_ARG(bool, true),
							  Q_ARG(bool, triggerSync));
}

void SyncManager::runOnSynchronized(const std::function<void (SyncManager::SyncState)> &resultFn, bool triggerSync)
{
	QMetaObject::invokeMethod(d->engine(), "syncForResult",
							  Q_ARG(SyncResultObject*, new SyncResultObject(resultFn, this)),
							  Q_ARG(bool, false),
							  Q_ARG(bool, triggerSync));
}

// ------------- Private Implementation -------------

SyncManagerPrivate::SyncManagerPrivate(const QString &setupName, SyncManager *q_ptr, bool blockingConstruct) :
	QObject(q_ptr),
	q(q_ptr),
	defaults(setupName),
	settings(defaults.createSettings(q_ptr)),
	cEnabled(true),
	cState(SyncManager::Initializing),
	cError()
{
	auto ngine = engine();
	auto rCon = ngine->remoteConnector();

	connect(ngine, &ExchangeEngine::lastErrorChanged,
			this, &SyncManagerPrivate::updateLastError);
	connect(ngine, &ExchangeEngine::stateChanged,
			this, &SyncManagerPrivate::updateSyncState);
	connect(rCon, &RemoteConnector::syncEnabledChanged,
			this, &SyncManagerPrivate::updateSyncEnabled);

	if(blockingConstruct) {
		QMetaObject::invokeMethod(ngine, "lastError",
								  Qt::BlockingQueuedConnection,
								  Q_RETURN_ARG(QString, cError));
		QMetaObject::invokeMethod(ngine, "state",
								  Qt::BlockingQueuedConnection,
								  Q_RETURN_ARG(QtDataSync::SyncManager::SyncState, cState));
		QMetaObject::invokeMethod(rCon, "isSyncEnabled",
								  Qt::BlockingQueuedConnection,
								  Q_RETURN_ARG(bool, cEnabled));
	} else {
		auto runFn = [this](ExchangeEngine *engine) {
			auto rCon = engine->remoteConnector();
			QMetaObject::invokeMethod(this, "updateLastError",
									  Q_ARG(QString, engine->lastError()));
			QMetaObject::invokeMethod(this, "updateSyncState",
									  Q_ARG(QtDataSync::SyncManager::SyncState, engine->state()));
			QMetaObject::invokeMethod(this, "updateSyncEnabled",
									  Q_ARG(bool, rCon->isSyncEnabled()));
		};
		QMetaObject::invokeMethod(SetupPrivate::engine(defaults.setupName()), "runInitFunc",
								  Q_ARG(QtDataSync::ExchangeEngine::RunFn, runFn));
	}
}

ExchangeEngine *SyncManagerPrivate::engine() const
{
	return SetupPrivate::engine(defaults.setupName());
}

RemoteConnector *SyncManagerPrivate::remoteConnector() const
{
	return engine()->remoteConnector();
}

void SyncManagerPrivate::updateSyncEnabled(bool syncEnabled)
{
	if(cEnabled != syncEnabled) {
		cEnabled = syncEnabled;
		emit q->syncEnabledChanged(syncEnabled);
	}
}

void SyncManagerPrivate::updateSyncState(SyncManager::SyncState state)
{
	if(cState != state) {
		cState = state;
		emit q->syncStateChanged(state);
	}
}

void SyncManagerPrivate::updateLastError(const QString &error)
{
	if(cError != error) {
		cError = error;
		emit q->lastErrorChanged(error);
	}
}
