#include "syncmanager.h"
#include "syncmanager_p.h"
#include "remoteconnector_p.h"
#include "setup_p.h"
#include "exchangeengine_p.h"
using namespace QtDataSync;

SyncManager::SyncManager(QObject *parent) :
	SyncManager(DefaultSetup, parent)
{}

SyncManager::SyncManager(const QString &setupName, QObject *parent) :
	QObject(parent),
	d(new SyncManagerPrivate(setupName, this))
{}

SyncManager::~SyncManager() {}

bool SyncManager::isSyncEnabled() const
{
	d->settings->sync();
	return d->settings->value(SyncManagerPrivate::keyEnabled, true).toBool();
}

void SyncManager::setSyncEnabled(bool syncEnabled)
{
	d->settings->sync();
	auto enabled = d->settings->value(SyncManagerPrivate::keyEnabled, true).toBool();
	if(enabled != syncEnabled) {
		d->settings->setValue(SyncManagerPrivate::keyEnabled, syncEnabled);
		d->settings->sync();
		d->reconnectEngine();
	}
	emit syncEnabledChanged(syncEnabled);
}

// ------------- Private Implementation -------------

const QString SyncManagerPrivate::keyEnabled(QStringLiteral("connector/enabled"));

SyncManagerPrivate::SyncManagerPrivate(const QString &setupName, SyncManager *q_ptr) :
	defaults(setupName),
	settings(defaults.createSettings(q_ptr))
{}

void SyncManagerPrivate::reconnectEngine()
{
	auto engine = SetupPrivate::engine(defaults.setupName());
	if(engine) {
		auto rCon = engine->remoteConnector();
		if(rCon)
			QMetaObject::invokeMethod(rCon, "reconnect", Qt::QueuedConnection);
	}
}
