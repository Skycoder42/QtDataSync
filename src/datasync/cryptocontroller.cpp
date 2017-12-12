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

const QString CryptoController::keySignScheme(QStringLiteral("scheme/signing"));
const QString CryptoController::keyCryptScheme(QStringLiteral("scheme/encryption"));
const QString CryptoController::keySignTemplate(QStringLiteral("device/%1/sign-key"));
const QString CryptoController::keyCryptTemplate(QStringLiteral("device/%1/crypt-key"));

CryptoController::CryptoController(const Defaults &defaults, QObject *parent) :
	Controller("crypto", defaults, parent),
	_keyStore(nullptr),
	_crypto(nullptr)
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

	_crypto = new ClientCrypto(this);
}

void CryptoController::finalize()
{
	if(_keyStore)
		_keyStore->closeStore();
}

ClientCrypto *CryptoController::crypto() const
{
	return _crypto;
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
		auto signScheme = settings()->value(keySignScheme).toByteArray();
		auto signKey = _keyStore->loadPrivateKey(keySignTemplate.arg(userId.toString()));
		if(signKey.isNull()) {
			logCritical() << "Unable to load private signing key from keystore";
			return false;
		}

		auto cryptScheme = settings()->value(keyCryptScheme).toByteArray();
		auto cryptKey = _keyStore->loadPrivateKey(keyCryptTemplate.arg(userId.toString()));
		if(cryptKey.isNull()) {
			logCritical() << "Unable to load private encryption key from keystore";
			return false;
		}

		_crypto->load(signScheme, signKey, cryptScheme, cryptKey);

		//TODO load and decrypt shared secret

		return true;
	} catch(CryptoPP::Exception &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to import private key"),
							  e);
	}
}

void CryptoController::createPrivateKeys(quint32 nonce)
{
	try {
		if(_crypto->rng().CanIncorporateEntropy())
			_crypto->rng().IncorporateEntropy((byte*)&nonce, sizeof(nonce));

		_crypto->generate((Setup::SignatureScheme)defaults().property(Defaults::SignScheme).toInt(),
						  defaults().property(Defaults::SignKeyParam),
						  (Setup::EncryptionScheme)defaults().property(Defaults::CryptScheme).toInt(),
						  defaults().property(Defaults::CryptKeyParam));

		logDebug().noquote().nospace() << "Generated new RSA keys"; //TODO common fingerprint?
	} catch(CryptoPP::Exception &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to generate private key"),
							  e);
	}
}

QStringList CryptoController::allKeys()
{
	return factory->allKeys();
}

// ------------- ClientCrypto Implementation -------------

ClientCrypto::ClientCrypto(QObject *parent) :
	AsymmetricCrypto(parent),
	_rng(true),
	_signKey(nullptr),
	_cryptKey(nullptr)
{}

void ClientCrypto::generate(Setup::SignatureScheme signScheme, const QVariant &signKeyParam, Setup::EncryptionScheme cryptScheme, const QVariant &cryptKeyParam)
{

}

void ClientCrypto::load(const QByteArray &signScheme, const QSslKey &signKeym, const QByteArray &cryptScheme, const QSslKey &cryptKey)
{

}

RandomNumberGenerator &ClientCrypto::rng()
{
	return _rng;
}

QSharedPointer<X509PublicKey> ClientCrypto::signKey() const
{
	return _signKey->createPublicKey();
}

QByteArray ClientCrypto::writeSignKey() const
{
	return writeKey(_signKey->createPublicKey());
}

QSharedPointer<X509PublicKey> ClientCrypto::cryptKey() const
{
	return _cryptKey->createPublicKey();
}

QByteArray ClientCrypto::writeCryptKey() const
{
	return writeKey(_cryptKey->createPublicKey());
}

const PKCS8PrivateKey &ClientCrypto::privateSignKey() const
{
	return _signKey->privateKeyRef();
}

const PKCS8PrivateKey &ClientCrypto::privateCryptKey() const
{
	return _cryptKey->privateKeyRef();
}

QByteArray ClientCrypto::sign(const QByteArray &message)
{
	return AsymmetricCrypto::sign(_signKey->privateKeyRef(), _rng, message);
}

QByteArray ClientCrypto::encrypt(const X509PublicKey &key, const QByteArray &message)
{
	return AsymmetricCrypto::encrypt(key, _rng, message);
}

QByteArray ClientCrypto::decrypt(const QByteArray &message)
{
	return AsymmetricCrypto::decrypt(_cryptKey->privateKeyRef(), _rng, message);
}

#define CC_CURVE(curve) case Setup::curve: return CryptoPP::ASN1::curve()

OID ClientCrypto::curveId(Setup::EllipticCurve curve)
{
	switch (curve) {
		CC_CURVE(secp112r1);
		CC_CURVE(secp128r1);
		CC_CURVE(secp160r1);
		CC_CURVE(secp192r1);
		CC_CURVE(secp224r1);
		CC_CURVE(secp256r1);
		CC_CURVE(secp384r1);
		CC_CURVE(secp521r1);

		CC_CURVE(brainpoolP160r1);
		CC_CURVE(brainpoolP192r1);
		CC_CURVE(brainpoolP224r1);
		CC_CURVE(brainpoolP256r1);
		CC_CURVE(brainpoolP320r1);
		CC_CURVE(brainpoolP384r1);
		CC_CURVE(brainpoolP512r1);

		CC_CURVE(secp112r2);
		CC_CURVE(secp128r2);
		CC_CURVE(secp160r2);
		CC_CURVE(secp160k1);
		CC_CURVE(secp192k1);
		CC_CURVE(secp224k1);
		CC_CURVE(secp256k1);

	default:
		Q_UNREACHABLE();
		break;
	}
}

#undef CC_CURVE

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
