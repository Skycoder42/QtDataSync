#include "enginethread.h"
#include "enginethread_p.h"
#include "asyncwatcher.h"
using namespace QtDataSync;

EngineThread::EngineThread(QScopedPointer<__private::SetupPrivate> &&setup, __private::SetupPrivate::ThreadInitFunc &&initFn, QObject *parent) :
	QThread{parent},
	d{new EngineThreadPrivate{}}
{
	d->init->setup.swap(setup);
	d->init->initFn = std::move(initFn);
	setTerminationEnabled(true);
}

EngineThread::~EngineThread() = default;

Engine *EngineThread::engine() const
{
	return d->engine.loadAcquire();
}

QFuture<AsyncWatcher*> EngineThread::createWatcher(QObject *parent) const
{
	QFutureInterface<AsyncWatcher*> fi;
	fi.reportStarted();
	if (const auto engine = d->engine.loadAcquire(); engine) {
		auto watcher = new AsyncWatcher{engine, parent};
		fi.reportFinished(&watcher);
	} else {
		connect(this, &EngineThread::engineCreated,
				parent, [fi, parent](Engine *engine) {
					auto fic = fi;
					auto watcher = new AsyncWatcher{engine, parent};
					fic.reportFinished(&watcher);
				});
	}
	return QFuture<AsyncWatcher*>{&fi};
}

void EngineThread::run()
{
	// init
	Engine engine{std::move(d->init->setup)};
	d->init->initFn(d->engine, this);
	d->init = std::nullopt;
	// notify frontend
	d->engine.storeRelease(&engine);
	Q_EMIT engineCreated(&engine);

	// run thread
	exec();

	// stop the engine and wait for it to finish
	if (engine.isRunning()) {
		engine.stop();
		engine.waitForStopped();
	}
	d->engine.storeRelease(nullptr);
}
