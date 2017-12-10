#include "cryptocontroller_p.h"
#include "logger.h"
#include "message_p.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDataStream>

#include <cryptoqq.h>
#include <qiodevicesink.h>
#include <qiodevicesource.h>
#include <qpluginfactory.h>

using namespace QtDataSync;
using namespace CryptoPP;
using Exception = QtDataSync::Exception;

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
		updateFingerprint();

		//TODO load and decrypt shared secret

		return true;
	} catch(CryptoPP::Exception &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to import private key"),
							  e);
	}
}

void CryptoController::createPrivateKey(quint32 nonce)
{
	try {
		if(_rng.CanIncorporateEntropy())
			_rng.IncorporateEntropy((byte*)&nonce, sizeof(nonce));
		_privateKey.GenerateRandomWithKeySize(_rng,
											  defaults().property(Defaults::RsaKeySize).toUInt());
		updateFingerprint();
		logDebug().noquote().nospace() << "Generated new RSA key: 0x" << _fingerprint.toHex();
	} catch(CryptoPP::Exception &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to generate private key"),
							  e);
	}
}

RSA::PublicKey CryptoController::publicKey() const
{
	return _privateKey;
}

void CryptoController::updateFingerprint()
{
	QByteArray pubData;
	QByteArraySink sink(pubData);
	_privateKey.DEREncodePublicKey(sink);
	_fingerprint = QCryptographicHash::hash(pubData, QCryptographicHash::Sha3_256);
}

QStringList CryptoController::allKeys()
{
	return factory->allKeys();
}

// ------------- Exceptions Implementation -------------

CryptoException::CryptoException(const Defaults &defaults, const QString &message, const CryptoPP::Exception &cExcept) :
	Exception(defaults, message),
	_exception(cExcept)
{}

CryptoPP::Exception CryptoException::cryptoPPException() const
{
	return _exception;
}

QString CryptoException::error() const
{
	return QString::fromStdString(_exception.GetWhat());
}

CryptoPP::Exception::ErrorType CryptoException::type() const
{
	return _exception.GetErrorType();
}

CryptoException::CryptoException(const CryptoException * const other) :
	Exception(other),
	_exception(other->_exception)
{}

QByteArray CryptoException::className() const noexcept
{
	return QTDATASYNC_EXCEPTION_NAME(CryptoException);
}

QString CryptoException::qWhat() const
{
	return Exception::qWhat() +
			QStringLiteral("\n\tCryptoPP::Error: %1"
						   "\n\tCryptoPP::Type: %2")
			.arg(error())
			.arg((int)type());
}

void CryptoException::raise() const
{
	throw (*this);
}

QException *CryptoException::clone() const
{
	return new CryptoException(this);
}
