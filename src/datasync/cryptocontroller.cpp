#include "cryptocontroller_p.h"
#include "logger.h"
#include "keystore.h"

#include <qpluginfactory.h>

using namespace QtDataSync;

typedef QPluginObjectFactory<KeyStorePlugin, KeyStore> Factory;
Q_GLOBAL_STATIC_WITH_ARGS(Factory, factory, (QLatin1String("keystores")))

#define QTDATASYNC_LOG _logger

CryptoController::CryptoController(const Defaults &defaults, QObject *parent) :
	QObject(parent),
	_defaults(defaults),
	_logger(_defaults.createLogger("crypto", this)),
	_settings(_defaults.createSettings(this, QStringLiteral("crypto"))),
	_keyStore(factory->createInstance(_defaults.property(Defaults::KeyStoreProvider).toString(), this))
{
	if(!_keyStore)
		logFatal(QStringLiteral("Failed to load desired keystore %1").arg(_defaults.property(Defaults::KeyStoreProvider).toString()));
}

QStringList CryptoController::allKeys()
{
	return factory->allKeys();
}
