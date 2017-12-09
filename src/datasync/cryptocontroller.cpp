#include "cryptocontroller_p.h"
#include "logger.h"

#include <cryptoqq.h>
#include <qpluginfactory.h>

using namespace QtDataSync;

typedef QPluginObjectFactory<KeyStorePlugin, KeyStore> Factory; //TODO make threadlocal instance?
Q_GLOBAL_STATIC_WITH_ARGS(Factory, factory, (QLatin1String("keystores")))

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

const QString CryptoController::keyPKeyTemplate(QStringLiteral("user/%1/private-key"));

CryptoController::CryptoController(const Defaults &defaults, QObject *parent) :
	Controller("crypto", defaults, parent),
	_keyStore(nullptr),
	_rng(true),
	_privateKey()
{}

void CryptoController::initialize()
{
	auto provider = defaults().property(Defaults::KeyStoreProvider).toString();
	_keyStore = factory->createInstance(provider, this);
	if(!_keyStore) {
		logCritical() << "Failed to load keystore"
					  << provider
					  << "- synchronization will be temporarily disabled";
	}
}

void CryptoController::finalize()
{
	if(_keyStore)
		_keyStore->closeStore();
}

bool CryptoController::canAccess() const
{
	return _keyStore && _keyStore->loadStore();
}

bool CryptoController::loadKeyMaterial(const QUuid &userId)
{
	if(!canAccess())
		return false;

	try {
		auto pKey = _keyStore->loadPrivateKey(keyPKeyTemplate.arg(userId.toString()));
		if(pKey.isNull()) {
			logCritical() << "Unable to load private user key from keystore";
			return false;
		}
		_privateKey = CryptoQQ::fromQtFormat(pKey);

		//TODO load and decrypt shared secret

		return true;
	} catch(CryptoPP::Exception &e) {
		logCritical() << "Failed to create CryptoPP private key with error:" << e.what();
		return false;
	}
}

QStringList CryptoController::allKeys()
{
	return factory->allKeys();
}
