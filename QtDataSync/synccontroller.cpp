#include "synccontroller.h"
#include "synccontroller_p.h"
#include "setup_p.h"
using namespace QtDataSync;

SyncController::SyncController(QObject *parent) :
	SyncController(Setup::DefaultSetup, parent)
{}

SyncController::SyncController(const QString &setupName, QObject *parent) :
	QObject(parent),
	d(new SyncControllerPrivate())
{
	d->engine = SetupPrivate::engine(setupName);
	Q_ASSERT_X(d->engine, Q_FUNC_INFO, "SyncController requires a valid setup!");
	connect(d->engine, &StorageEngine::syncStateChanged,
			this, &SyncController::syncStateChanged,
			Qt::QueuedConnection);
}

SyncController::~SyncController() {}

SyncController::SyncState SyncController::syncState() const
{
	return d->engine->syncState();
}

void SyncController::triggerSync()
{
	Q_UNIMPLEMENTED();
}
