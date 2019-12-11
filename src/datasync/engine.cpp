#include "engine.h"
#include "engine_p.h"
using namespace QtDataSync;

Engine::Engine(QObject *parent) :
	QObject{*new EnginePrivate{}, parent}
{}

void Engine::setupFirebase(const QByteArray &projectId, const QByteArray &webApiKey)
{

}
