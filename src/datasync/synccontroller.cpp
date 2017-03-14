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
	QMetaObject::invokeMethod(d->engine, "triggerSync");
}

void SyncController::triggerSyncWithResult(std::function<void (SyncState)> resultFn)
{
	auto receiver = new QObject(this);//dummy to disconnect after one call
	connect(this, &SyncController::syncStateChanged, receiver, [=](SyncState state){
		if(state == Loading || state == Syncing)
			return;
		resultFn(state);
		receiver->deleteLater();
	});
	triggerSync();
}
