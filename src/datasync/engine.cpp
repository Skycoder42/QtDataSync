#include "engine.h"
#include "engine_p.h"
using namespace QtDataSync;

Engine::Engine(QObject *parent) :
	QObject{*new EnginePrivate{}, parent}
{}

IAuthenticator *Engine::authenticator() const
{
	Q_D(const Engine);
	return d->authenticator;
}

void Engine::start()
{
	Q_D(Engine);
	QMetaObject::invokeMethod(d->authenticator, "signIn", Qt::QueuedConnection);
}
