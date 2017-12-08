#include "cryptocontroller_p.h"
#include "logger.h"

#include <qpluginfactory.h>

using namespace QtDataSync;

typedef QPluginObjectFactory<KeyStorePlugin, KeyStore> Factory; //TODO make threadlocal instance?
Q_GLOBAL_STATIC_WITH_ARGS(Factory, factory, (QLatin1String("keystores")))

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

CryptoController::CryptoController(const Defaults &defaults, QObject *parent) :
	Controller("crypto", defaults, parent),
	_keyStore(nullptr)
{}

void CryptoController::initialize()
{
	auto provider = defaults().property(Defaults::KeyStoreProvider).toString();
	_keyStore = factory->createInstance(provider, this);
	if(!_keyStore)
		logFatal(QStringLiteral("Failed to load desired keystore %1").arg(provider));
}

QStringList CryptoController::allKeys()
{
	return factory->allKeys();
}
