#include "cryptocontroller_p.h"
#include "logger.h"
#include "message_p.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDataStream>

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

QByteArray CryptoController::fingerprint() const
{
	if(_fingerprint.isEmpty()) {
		try {
			QCryptographicHash hash(QCryptographicHash::Sha3_256);
			hash.addData(_crypto->signatureScheme());
			hash.addData(_crypto->writeSignKey());
			hash.addData(_crypto->encryptionScheme());
			hash.addData(_crypto->writeCryptKey());
			_fingerprint = hash.result();
		} catch(CryptoPP::Exception &e) {
			throw CryptoException(defaults(),
								  QStringLiteral("Failed to generate device fingerprint"),
								  e);
		}
	}

	return _fingerprint;
}

bool CryptoController::canAccessStore() const
{
	return _keyStore && _keyStore->loadStore();
}

bool CryptoController::loadKeyMaterial(const QUuid &deviceId)
{
	if(!canAccessStore())
		return false;

	try {
		_fingerprint.clear();

		auto signScheme = settings()->value(keySignScheme).toByteArray();
		auto signKey = _keyStore->loadPrivateKey(keySignTemplate.arg(deviceId.toString()));
		if(signKey.isNull()) {
			logCritical() << "Unable to load private signing key from keystore";
			return false;
		}

		auto cryptScheme = settings()->value(keyCryptScheme).toByteArray();
		auto cryptKey = _keyStore->loadPrivateKey(keyCryptTemplate.arg(deviceId.toString()));
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
		_fingerprint.clear();

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

QStringList CryptoController::allKeystoreKeys()
{
	return factory->allKeys();
}

// ------------- KeyScheme class definitions -------------

template <typename TScheme>
class RsaKeyScheme : public ClientCrypto::KeyScheme
{
public:
	QByteArray name() const override;
	void createPrivateKey(RandomNumberGenerator &rng, const QVariant &keyParam) override;
	void loadPrivateKey(const QSslKey &key) override;
	QSslKey savePrivateKey() override;
	const PKCS8PrivateKey &privateKeyRef() const override;
	QSharedPointer<X509PublicKey> createPublicKey() const override;

private:
	typename TScheme::PrivateKey _key;
};

template <typename TScheme>
class EccKeyScheme : public ClientCrypto::KeyScheme
{
public:
	QByteArray name() const override;
	void createPrivateKey(RandomNumberGenerator &rng, const QVariant &keyParam) override;
	void loadPrivateKey(const QSslKey &key) override;
	QSslKey savePrivateKey() override;
	const PKCS8PrivateKey &privateKeyRef() const override;
	QSharedPointer<X509PublicKey> createPublicKey() const override;

private:
	typename TScheme::PrivateKey _key;
};

// ------------- ClientCrypto Implementation -------------

ClientCrypto::ClientCrypto(QObject *parent) :
	AsymmetricCrypto(parent),
	_rng(true),
	_signKey(nullptr),
	_cryptKey(nullptr)
{}

void ClientCrypto::generate(Setup::SignatureScheme signScheme, const QVariant &signKeyParam, Setup::EncryptionScheme cryptScheme, const QVariant &cryptKeyParam)
{
	//first: clean all
	resetSchemes();
	_signKey.reset();
	_cryptKey.reset();

	//load all schemes
	setSignatureKey(signScheme);
	setSignatureScheme(_signKey->name());
	setEncryptionKey(cryptScheme);
	setEncryptionScheme(_cryptKey->name());

	if(_signKey->name() != signatureScheme())
		throw CryptoPP::Exception(CryptoPP::Exception::OTHER_ERROR, "Signing key scheme does not match signature scheme");
	if(_cryptKey->name() != encryptionScheme())
		throw CryptoPP::Exception(CryptoPP::Exception::OTHER_ERROR, "Crypting key scheme does not match encryption scheme");

	_signKey->createPrivateKey(_rng, signKeyParam);
	if(!_signKey->privateKeyRef().Validate(_rng, 3))
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_DATA_FORMAT, "Signature key failed validation");
	_cryptKey->createPrivateKey(_rng, cryptKeyParam);
	if(!_cryptKey->privateKeyRef().Validate(_rng, 3))
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_DATA_FORMAT, "Signature key failed validation");
}

void ClientCrypto::load(const QByteArray &signScheme, const QSslKey &signKey, const QByteArray &cryptScheme, const QSslKey &cryptKey)
{
	//first: clean all
	resetSchemes();
	_signKey.reset();
	_cryptKey.reset();

	//load all schemes
	setSignatureKey(signScheme);
	setSignatureScheme(signScheme);
	setEncryptionKey(cryptScheme);
	setEncryptionScheme(cryptScheme);

	if(_signKey->name() != signatureScheme())
		throw CryptoPP::Exception(CryptoPP::Exception::OTHER_ERROR, "Signing key scheme does not match signature scheme");
	if(_cryptKey->name() != encryptionScheme())
		throw CryptoPP::Exception(CryptoPP::Exception::OTHER_ERROR, "Crypting key scheme does not match encryption scheme");

	_signKey->loadPrivateKey(signKey);
	if(!_signKey->privateKeyRef().Validate(_rng, 3))
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_DATA_FORMAT, "Signature key failed validation");
	_cryptKey->loadPrivateKey(cryptKey);
	if(!_cryptKey->privateKeyRef().Validate(_rng, 3))
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_DATA_FORMAT, "Signature key failed validation");
}

RandomNumberGenerator &ClientCrypto::rng()
{
	return _rng;
}

QSharedPointer<X509PublicKey> ClientCrypto::readKey(bool signKey, const QByteArray &data)
{
	return AsymmetricCrypto::readKey(signKey, _rng, data);
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

void ClientCrypto::setSignatureKey(const QByteArray &name)
{
	auto stdStr = name.toStdString();
	if(stdStr == RsassScheme::StaticAlgorithmName())
		_signKey.reset(new RsaKeyScheme<RsassScheme>());
	else if(stdStr == EcdsaScheme::StaticAlgorithmName())
		_signKey.reset(new EccKeyScheme<EcdsaScheme>());
	else if(stdStr == EcnrScheme::StaticAlgorithmName())
		_signKey.reset(new EccKeyScheme<EcnrScheme>());
	else
		throw CryptoPP::Exception(CryptoPP::Exception::NOT_IMPLEMENTED, "Signature Scheme \"" + stdStr + "\" not supported");
}

void ClientCrypto::setSignatureKey(Setup::SignatureScheme scheme)
{
	switch (scheme) {
	case Setup::RSA_PSS_SHA3_512:
		setSignatureKey(QByteArray::fromStdString(RsassScheme::StaticAlgorithmName()));
		break;
	case Setup::ECDSA_ECP_SHA3_512:
		setSignatureKey(QByteArray::fromStdString(EcdsaScheme::StaticAlgorithmName()));
		break;
	case Setup::ECNR_ECP_SHA3_512:
		setSignatureKey(QByteArray::fromStdString(EcnrScheme::StaticAlgorithmName()));
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void ClientCrypto::setEncryptionKey(const QByteArray &name)
{
	auto stdStr = name.toStdString();
	if(stdStr == RsaesScheme::StaticAlgorithmName())
		_cryptKey.reset(new RsaKeyScheme<RsaesScheme>());
	else
		throw CryptoPP::Exception(CryptoPP::Exception::NOT_IMPLEMENTED, "Encryption Scheme \"" + stdStr + "\" not supported");
}

void ClientCrypto::setEncryptionKey(Setup::EncryptionScheme scheme)
{
	switch (scheme) {
	case Setup::RSA_OAEP_SHA3_512:
		setEncryptionKey(QByteArray::fromStdString(RsaesScheme::StaticAlgorithmName()));
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

#undef CC_CURVE

// ------------- Generic KeyScheme Implementation -------------

template <typename TScheme>
QByteArray RsaKeyScheme<TScheme>::name() const
{
	return QByteArray::fromStdString(TScheme::StaticAlgorithmName());
}

template <typename TScheme>
void RsaKeyScheme<TScheme>::createPrivateKey(RandomNumberGenerator &rng, const QVariant &keyParam)
{
	if(keyParam.type() != QVariant::Int)
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_ARGUMENT, "keyParam must be an unsigned integer");
	_key.GenerateRandomWithKeySize(rng, keyParam.toUInt());
}

template <typename TScheme>
void RsaKeyScheme<TScheme>::loadPrivateKey(const QSslKey &key)
{
	QByteArraySource source(key.toDer(), true);
	_key.BERDecodePrivateKey(source, false, 0);
}

template<typename TScheme>
QSslKey RsaKeyScheme<TScheme>::savePrivateKey()
{
	QByteArray data;
	QByteArraySink sink(data);
	_key.DEREncodePrivateKey(sink);
	return QSslKey(data, QSsl::Rsa, QSsl::Der);
}

template <typename TScheme>
const PKCS8PrivateKey &RsaKeyScheme<TScheme>::privateKeyRef() const
{
	return _key;
}

template <typename TScheme>
QSharedPointer<X509PublicKey> RsaKeyScheme<TScheme>::createPublicKey() const
{
	return QSharedPointer<typename TScheme::PublicKey>::create(_key);
}



template <typename TScheme>
QByteArray EccKeyScheme<TScheme>::name() const
{
	return QByteArray::fromStdString(TScheme::StaticAlgorithmName());
}

template <typename TScheme>
void EccKeyScheme<TScheme>::createPrivateKey(RandomNumberGenerator &rng, const QVariant &keyParam)
{
	if(keyParam.type() != QVariant::Int)
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_ARGUMENT, "keyParam must be a Setup::EllipticCurve");
	auto curve = ClientCrypto::curveId((Setup::EllipticCurve)keyParam.toInt());
	_key.Initialize(rng, curve);
}

template <typename TScheme>
void EccKeyScheme<TScheme>::loadPrivateKey(const QSslKey &key)
{
	QByteArraySource source(key.toDer(), true);
	_key.BERDecodePrivateKey(source, false, 0);
}

template<typename TScheme>
QSslKey EccKeyScheme<TScheme>::savePrivateKey()
{
	QByteArray data;
	QByteArraySink sink(data);
	_key.DEREncodePrivateKey(sink);
	return QSslKey(data, QSsl::Ec, QSsl::Der);
}

template <typename TScheme>
const PKCS8PrivateKey &EccKeyScheme<TScheme>::privateKeyRef() const
{
	return _key;
}

template <typename TScheme>
QSharedPointer<X509PublicKey> EccKeyScheme<TScheme>::createPublicKey() const
{
	auto key = QSharedPointer<typename TScheme::PublicKey>::create();
	_key.MakePublicKey(*key);
	return key;
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
