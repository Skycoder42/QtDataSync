#include "engine.h"
#include "engine_p.h"
using namespace QtDataSync;

IAuthenticator *Engine::authenticator() const
{
	Q_D(const Engine);
	return d->setup->authenticator;
}

void Engine::start()
{
	Q_D(Engine);
	QMetaObject::invokeMethod(d->setup->authenticator, "signIn", Qt::QueuedConnection);
}

Engine::Engine(QScopedPointer<SetupPrivate> &&setup, QObject *parent) :
	QObject{*new EnginePrivate{}, parent}
{
	Q_D(Engine);
	d->setup.swap(setup);
	d->setup->finializeForEngine(this);
}

const SetupPrivate *EnginePrivate::setupFor(const Engine *engine)
{
	return engine->d_func()->setup.data();
}
