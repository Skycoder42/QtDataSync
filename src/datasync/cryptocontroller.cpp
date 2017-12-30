#include "cryptocontroller_p.h"
#include "logger.h"
#include "message_p.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDataStream>
#include <QtCore/QJsonDocument>

#include <cryptopp/eax.h>
#include <cryptopp/gcm.h>
#include <cryptopp/aes.h>
#include <cryptopp/idea.h>
#include <cryptopp/twofish.h>
#include <cryptopp/serpent.h>

#include <qiodevicesink.h>
#include <qiodevicesource.h>
#include <qpluginfactory.h>

using namespace QtDataSync;
using namespace CryptoPP;
using Exception = QtDataSync::Exception;
#ifndef __clang__
using CppException = CryptoPP::Exception;
#else
using CppException = std::exception;
#endif

namespace {

class ExtendedFactory : public QPluginFactory<KeyStorePlugin>
{
public:
	ExtendedFactory();

	bool isAvailable(const QString &provider);
	KeyStore *createInstance(const QString &key, const Defaults &defaults, QObject *parent = nullptr);
};

}

Q_GLOBAL_STATIC(ExtendedFactory, factory)

// ------------- CipherScheme class definitions -------------

template <template<class> class TScheme, class TCipher>
class StandardCipherScheme : public CryptoController::CipherScheme
{
public:
	typedef TScheme<TCipher> Scheme;

	QByteArray name() const override;
	quint32 defaultKeyLength() const override;
	quint32 ivLength() const override;
	quint32 toKeyLength(quint32 length) const override;
	QSharedPointer<AuthenticatedSymmetricCipher> encryptor() const override;
	QSharedPointer<AuthenticatedSymmetricCipher> decryptor() const override;
	QSharedPointer<MessageAuthenticationCode> cmac() const override;
};

template <typename T>
using GCM1 = GCM<T>;

// ------------- KeyScheme class definitions -------------

template <typename TScheme>
class RsaKeyScheme : public ClientCrypto::KeyScheme
{
public:
	QByteArray name() const override;
	void createPrivateKey(RandomNumberGenerator &rng, const QVariant &keyParam) override;
	PKCS8PrivateKey &privateKeyRef() override;
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
	PKCS8PrivateKey &privateKeyRef() override;
	QSharedPointer<X509PublicKey> createPublicKey() const override;

private:
	typename TScheme::PrivateKey _key;
};

// ------------- CryptoController Implementation -------------

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

const QString CryptoController::keyKeystore(QStringLiteral("keystore"));
const QString CryptoController::keySignScheme(QStringLiteral("scheme/signing"));
const QString CryptoController::keyCryptScheme(QStringLiteral("scheme/encryption"));
const QString CryptoController::keyLocalSymKey(QStringLiteral("localkey"));
const QString CryptoController::keySymKeysTemplate(QStringLiteral("scheme/key/%1"));

const QString CryptoController::keySignKeyTemplate(QStringLiteral("%1/signing"));
const QString CryptoController::keyCryptKeyTemplate(QStringLiteral("%1/encryption"));
const QString CryptoController::keyKeyFileTemplate(QStringLiteral("key_%1.enc"));

CryptoController::CryptoController(const Defaults &defaults, QObject *parent) :
	Controller("crypto", defaults, parent),
	_keyStore(nullptr),
	_asymCrypto(nullptr),
	_loadedChiphers(),
	_localCipher(0)
{}

QStringList CryptoController::allKeystoreKeys()
{
	return factory->allKeys();
}

QStringList CryptoController::availableKeystoreKeys()
{
	QStringList keys;
	foreach(auto key, factory->allKeys()) {
		if(factory->isAvailable(key))
			keys.append(key);
	}
	return keys;
}

bool CryptoController::keystoreAvailable(const QString &provider)
{
	return factory->isAvailable(provider);
}

void CryptoController::initialize(const QVariantHash &params)
{
	Q_UNUSED(params)
	_asymCrypto = new ClientCrypto(this);
}

void CryptoController::finalize()
{
	closeStore();
}

ClientCrypto *CryptoController::crypto() const
{
	return _asymCrypto;
}

QByteArray CryptoController::fingerprint() const
{
	if(_fingerprint.isEmpty()) {
		try {
			QCryptographicHash hash(QCryptographicHash::Sha3_256);
			hash.addData(_asymCrypto->signatureScheme());
			hash.addData(_asymCrypto->writeSignKey());
			hash.addData(_asymCrypto->encryptionScheme());
			hash.addData(_asymCrypto->writeCryptKey());
			_fingerprint = hash.result();
		} catch(CppException &e) {
			throw CryptoException(defaults(),
								  QStringLiteral("Failed to generate device fingerprint"),
								  e);
		}
	}

	return _fingerprint;
}

bool CryptoController::acquireStore(bool existing)
{
	closeStore();
	QString provider;
	if(existing)
		provider = settings()->value(keyKeystore).toString();
	else {
		provider = defaults().property(Defaults::KeyStoreProvider).toString();
		if(provider.isNull())
			provider = Setup::defaultKeystoreProvider();
	}

	if(_keyStore) {
		if(_keyStore->providerName() == provider) //keystore is already loaded - no need to delete and load again
			return true;
		else {
			_keyStore->deleteLater();
			_keyStore = nullptr;
		}
	}

	_keyStore = factory->createInstance(provider, defaults(), this);
	if(!_keyStore) {
		logCritical() << "Keystore"
					  << provider
					  << "not available - synchronization will be temporarily disabled";
		return false;
	}

	return true;
}

void CryptoController::loadKeyMaterial(const QUuid &deviceId)
{
	try {
		ensureStoreOpen();
		_fingerprint.clear();

		auto signScheme = settings()->value(keySignScheme).toByteArray();
		auto signKey = _keyStore->loadPrivateKey(keySignKeyTemplate.arg(deviceId.toString()));
		if(signKey.isNull())
			throw KeyStoreException(_keyStore, QStringLiteral("Unable to load private signing key from keystore"));

		auto cryptScheme = settings()->value(keyCryptScheme).toByteArray();
		auto cryptKey = _keyStore->loadPrivateKey(keyCryptKeyTemplate.arg(deviceId.toString()));
		if(cryptKey.isNull())
			throw KeyStoreException(_keyStore, QStringLiteral("Unable to load private encryption key from keystore"));

		_asymCrypto->load(signScheme, signKey, cryptScheme, cryptKey);

		_localCipher = settings()->value(keyLocalSymKey, 0).toUInt();
		getInfo(_localCipher);

		logDebug() << "Loaded keys for" << deviceId;
		closeStore();
#ifdef __clang__
	} catch(QException &e) { //prevent catching the std::exception
		throw;
#endif
	} catch(CppException &e) {
		closeStore();
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to import private key"),
							  e);
	}
}

void CryptoController::clearKeyMaterial()
{
	_fingerprint.clear();
	_asymCrypto->reset();
	_loadedChiphers.clear();
	_localCipher = 0;
	closeStore();
	logDebug() << "Cleared all key material";
}

void CryptoController::createPrivateKeys(const QByteArray &nonce)
{
	try {
		_fingerprint.clear();

		if(_asymCrypto->rng().CanIncorporateEntropy())
			_asymCrypto->rng().IncorporateEntropy((const byte*)nonce.constData(), nonce.size());

		//generate private signature and encryption keys
		_asymCrypto->generate((Setup::SignatureScheme)defaults().property(Defaults::SignScheme).toInt(),
							  defaults().property(Defaults::SignKeyParam),
							  (Setup::EncryptionScheme)defaults().property(Defaults::CryptScheme).toInt(),
							  defaults().property(Defaults::CryptKeyParam));

		//create symmetric cipher and the key
		CipherInfo info;
		createScheme((Setup::CipherScheme)defaults().property(Defaults::SymScheme).toInt(),
					 info.scheme);
		auto keySize = defaults().property(Defaults::SymKeyParam).toUInt();
		if(keySize == 0)
			keySize = info.scheme->defaultKeyLength();
		else
			keySize = info.scheme->toKeyLength(keySize);
		info.key.CleanNew(keySize);
		_asymCrypto->rng().GenerateBlock(info.key.data(), info.key.size());
		_localCipher = 0;
		_loadedChiphers.insert(_localCipher, info);

#ifndef QT_NO_DEBUG
		logDebug().noquote() << "Generated new keys. Public keys fingerprint:"
							 << fingerprint().toHex();
#else
		logDebug() << "Generated new keys";
#endif
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to generate private key"),
							  e);
	}
}

void CryptoController::storePrivateKeys(const QUuid &deviceId) const
{
	try {
		ensureStoreOpen();

		settings()->setValue(keySignScheme, _asymCrypto->signatureScheme());
		_keyStore->storePrivateKey(keySignKeyTemplate.arg(deviceId.toString()),
								   _asymCrypto->savePrivateSignKey());

		settings()->setValue(keyCryptScheme, _asymCrypto->encryptionScheme());
		_keyStore->storePrivateKey(keyCryptKeyTemplate.arg(deviceId.toString()),
								   _asymCrypto->savePrivateCryptKey());

		//store with key index 0, as the initial key, that will never be managed by the server. Server will always start counting at 1
		storeCipherKey(_localCipher);
		settings()->setValue(keyLocalSymKey, _localCipher);
		logDebug() << "Stored keys for" << deviceId;
		closeStore();
	} catch(CppException &e) {
		closeStore();
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to generate private key"),
							  e);
	}
}

std::tuple<quint32, QByteArray, QByteArray> CryptoController::encrypt(const QByteArray &plain)
{
	try {
		auto info = getInfo(_localCipher);
		QByteArray salt(info.scheme->ivLength(), Qt::Uninitialized);
		_asymCrypto->rng().GenerateBlock((byte*)salt.data(), salt.size());

		auto enc = info.scheme->encryptor();
		enc->SetKeyWithIV(info.key.data(), info.key.size(),
						  (const byte*)salt.constData(), salt.size());

		QByteArray cipher;
		QByteArraySource(plain, true,
			new AuthenticatedEncryptionFilter(*(enc.data()),
				new QByteArraySink(cipher)
			) // AuthenticatedEncryptionFilter
		); // QByteArraySource

		return std::tuple<quint32, QByteArray, QByteArray>{_localCipher, salt, cipher};
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to encrypt data for upload"),
							  e);
	}
}

QByteArray CryptoController::decrypt(quint32 keyIndex, const QByteArray &salt, const QByteArray &cipher) const
{
	try {
		auto info = getInfo(keyIndex);

		auto dec = info.scheme->decryptor();
		dec->SetKeyWithIV(info.key.data(), info.key.size(),
						  (const byte*)salt.constData(), salt.size());

		QByteArray plain;
		QByteArraySource(cipher, true,
			new AuthenticatedDecryptionFilter(*(dec.data()),
				new QByteArraySink(plain)
			) // AuthenticatedDecryptionFilter
		); // QByteArraySource

		return plain;
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to decrypt downloaded data"),
							  e);
	}
}

std::tuple<quint32, QByteArray> CryptoController::createCmac(const QByteArray &data) const
{
	try {
		auto info = getInfo(_localCipher);

		auto cmac = info.scheme->cmac();
		cmac->SetKey(info.key.data(), info.key.size());

		QByteArray mac;
		QByteArraySource (data, true,
			new HashFilter(*(cmac.data()),
				new QByteArraySink(mac)
			) // HashFilter
		); // QByteArraySource

		return std::tuple<quint32, QByteArray>{_localCipher, mac};
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to create CMAC"),
							  e);
	}
}

void CryptoController::verifyCmac(quint32 keyIndex, const QByteArray &data, const QByteArray &mac) const
{
	try {
		auto info = getInfo(keyIndex);

		auto cmac = info.scheme->cmac();
		cmac->SetKey(info.key.data(), info.key.size());

		QByteArraySource (data + mac, true,
			new HashVerificationFilter(*(cmac.data()), nullptr, HashVerificationFilter::THROW_EXCEPTION | HashVerificationFilter::HASH_AT_END) // HashFilter
		); // QByteArraySource
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to verify CMAC"),
							  e);
	}
}

void CryptoController::createScheme(const QByteArray &name, QSharedPointer<CipherScheme> &ptr)
{
	auto stdStr = name.toStdString();
	if(stdStr == EAX<AES>::Encryption::StaticAlgorithmName())
		ptr.reset(new StandardCipherScheme<EAX, AES>());
	else if(stdStr == GCM1<AES>::Encryption::StaticAlgorithmName())
		ptr.reset(new StandardCipherScheme<GCM1, AES>());
	else if(stdStr == EAX<Twofish>::Encryption::StaticAlgorithmName())
		ptr.reset(new StandardCipherScheme<EAX, Twofish>());
	else if(stdStr == GCM1<Twofish>::Encryption::StaticAlgorithmName())
		ptr.reset(new StandardCipherScheme<GCM1, Twofish>());
	else if(stdStr == EAX<Serpent>::Encryption::StaticAlgorithmName())
		ptr.reset(new StandardCipherScheme<EAX, Serpent>());
	else if(stdStr == GCM1<Serpent>::Encryption::StaticAlgorithmName())
		ptr.reset(new StandardCipherScheme<GCM1, Serpent>());
	else if(stdStr == EAX<IDEA>::Encryption::StaticAlgorithmName())
		ptr.reset(new StandardCipherScheme<EAX, IDEA>());
	else
		throw CryptoPP::Exception(CryptoPP::Exception::NOT_IMPLEMENTED, "Symmetric Cipher Scheme \"" + stdStr + "\" not supported");
}

void CryptoController::createScheme(Setup::CipherScheme scheme, QSharedPointer<CipherScheme> &ptr)
{
	switch (scheme) {
	case Setup::AES_EAX:
		createScheme(QByteArray::fromStdString(EAX<AES>::Encryption::StaticAlgorithmName()), ptr);
		break;
	case Setup::AES_GCM:
		createScheme(QByteArray::fromStdString(GCM<AES>::Encryption::StaticAlgorithmName()), ptr);
		break;
	case Setup::TWOFISH_EAX:
		createScheme(QByteArray::fromStdString(EAX<Twofish>::Encryption::StaticAlgorithmName()), ptr);
		break;
	case Setup::TWOFISH_GCM:
		createScheme(QByteArray::fromStdString(EAX<Twofish>::Encryption::StaticAlgorithmName()), ptr);
		break;
	case Setup::SERPENT_EAX:
		createScheme(QByteArray::fromStdString(EAX<Serpent>::Encryption::StaticAlgorithmName()), ptr);
		break;
	case Setup::SERPENT_GCM:
		createScheme(QByteArray::fromStdString(EAX<Serpent>::Encryption::StaticAlgorithmName()), ptr);
		break;
	case Setup::IDEA_EAX:
		createScheme(QByteArray::fromStdString(EAX<IDEA>::Encryption::StaticAlgorithmName()), ptr);
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void CryptoController::ensureStoreOpen() const
{
	if(_keyStore) {
		if(!_keyStore->isOpen())
			_keyStore->openStore();
	} else
		throw Exception(defaults(), QStringLiteral("No keystore available"));
}

void CryptoController::closeStore() const
{
	try {
		if(_keyStore && _keyStore->isOpen())
			_keyStore->closeStore();
	} catch(QException &e) {
		logCritical() << "Failed to close keystore with error:" << e.what();
	}
}

void CryptoController::storeCipherKey(quint32 keyIndex) const
{
	auto keyDir = keysDir();
	auto info = getInfo(keyIndex);
	auto encData = _asymCrypto->encrypt(_asymCrypto->cryptKey(),
										QByteArray::fromRawData((const char*)info.key.data(), (int)info.key.size()));
	QFile keyFile(keyDir.absoluteFilePath(keyKeyFileTemplate.arg(keyIndex)));
	if(!keyFile.open(QIODevice::WriteOnly))
		throw CryptoPP::Exception(CryptoPP::Exception::IO_ERROR, keyFile.errorString().toStdString());
	keyFile.write(encData);
	keyFile.close();

	settings()->setValue(keySymKeysTemplate.arg(keyIndex), info.scheme->name());
}

const CryptoController::CipherInfo &CryptoController::getInfo(quint32 keyIndex) const
{
	if(!_loadedChiphers.contains(keyIndex)) {
		auto keyDir = keysDir();
		CipherInfo info;

		createScheme(settings()->value(keySymKeysTemplate.arg(keyIndex)).toByteArray(), info.scheme);

		QFile keyFile(keyDir.absoluteFilePath(keyKeyFileTemplate.arg(keyIndex)));
		if(!keyFile.open(QIODevice::ReadOnly))
			throw CryptoPP::Exception(CryptoPP::Exception::IO_ERROR, keyFile.errorString().toStdString());
		auto encData = keyFile.readAll();
		keyFile.close();

		auto key = _asymCrypto->decrypt(encData);
		info.key.Assign((const byte*)key.constData(), key.size());
		memset(key.data(), 0, key.size());

		//test if the key is of valid length
		if(info.key.size() != info.scheme->toKeyLength((quint32)info.key.size()))
			throw CryptoPP::Exception(CryptoPP::Exception::OTHER_ERROR, "Key size is not valid for cipher scheme " + info.scheme->name().toStdString());
		_loadedChiphers.insert(keyIndex, info);
	}

	return _loadedChiphers[keyIndex];
}

QDir CryptoController::keysDir() const
{
	auto keyDirName = QStringLiteral("keys");
	auto keyDir = defaults().storageDir();
	if(!keyDir.mkpath(keyDirName) || !keyDir.cd(keyDirName))
		throw CryptoPP::Exception(CryptoPP::Exception::IO_ERROR, "Failed to create keys directory");
	return keyDir;
}

// ------------- ClientCrypto Implementation -------------

ClientCrypto::ClientCrypto(QObject *parent) :
	AsymmetricCrypto(parent),
#ifdef QT_NO_DEBUG
	_rng(true, 32),
#else //fast rng pooling for debug builds
	_rng(false),
#endif
	_signKey(nullptr),
	_cryptKey(nullptr)
{}

void ClientCrypto::generate(Setup::SignatureScheme signScheme, const QVariant &signKeyParam, Setup::EncryptionScheme cryptScheme, const QVariant &cryptKeyParam)
{
	//first: clean all
	reset();

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

void ClientCrypto::load(const QByteArray &signScheme, const QByteArray &signKey, const QByteArray &cryptScheme, const QByteArray &cryptKey)
{
	//first: clean all
	reset();

	//load all schemes
	setSignatureKey(signScheme);
	setSignatureScheme(signScheme);
	setEncryptionKey(cryptScheme);
	setEncryptionScheme(cryptScheme);

	if(_signKey->name() != signatureScheme())
		throw CryptoPP::Exception(CryptoPP::Exception::OTHER_ERROR, "Signing key scheme does not match signature scheme");
	if(_cryptKey->name() != encryptionScheme())
		throw CryptoPP::Exception(CryptoPP::Exception::OTHER_ERROR, "Crypting key scheme does not match encryption scheme");

	loadKey(_signKey->privateKeyRef(), signKey);
	if(!_signKey->privateKeyRef().Validate(_rng, 3))
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_DATA_FORMAT, "Signature key failed validation");
	loadKey(_cryptKey->privateKeyRef(), cryptKey);
	if(!_cryptKey->privateKeyRef().Validate(_rng, 3))
		throw CryptoPP::Exception(CryptoPP::Exception::INVALID_DATA_FORMAT, "Signature key failed validation");
}

void ClientCrypto::reset()
{
	resetSchemes();
	_signKey.reset();
	_cryptKey.reset();
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

QByteArray ClientCrypto::savePrivateSignKey() const
{
	return saveKey(_signKey->privateKeyRef());
}

const PKCS8PrivateKey &ClientCrypto::privateCryptKey() const
{
	return _cryptKey->privateKeyRef();
}

QByteArray ClientCrypto::savePrivateCryptKey() const
{
	return saveKey(_cryptKey->privateKeyRef());
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

void ClientCrypto::loadKey(PKCS8PrivateKey &key, const QByteArray &data) const
{
	QByteArraySource source(data, true);
	key.Load(source);
}

QByteArray ClientCrypto::saveKey(const PKCS8PrivateKey &key) const
{
	QByteArray data;
	QByteArraySink sink(data);
	key.Save(sink);
	return data;
}

// ------------- (Generic) CipherScheme Implementation -------------

template <template<class> class TScheme, class TCipher>
QByteArray StandardCipherScheme<TScheme, TCipher>::name() const
{
	return QByteArray::fromStdString(Scheme::Encryption::StaticAlgorithmName());
}

template <template<class> class TScheme, class TCipher>
quint32 StandardCipherScheme<TScheme, TCipher>::defaultKeyLength() const
{
	return TCipher::MAX_KEYLENGTH;
}

template <template<class> class TScheme, class TCipher>
quint32 StandardCipherScheme<TScheme, TCipher>::ivLength() const
{
	return TCipher::BLOCKSIZE;
}

template <template<class> class TScheme, class TCipher>
quint32 StandardCipherScheme<TScheme, TCipher>::toKeyLength(quint32 length) const
{
	return (quint32)TCipher::StaticGetValidKeyLength(length);
}

template <template<class> class TScheme, class TCipher>
QSharedPointer<AuthenticatedSymmetricCipher> StandardCipherScheme<TScheme, TCipher>::encryptor() const
{
	return QSharedPointer<typename Scheme::Encryption>::create();
}

template <template<class> class TScheme, class TCipher>
QSharedPointer<AuthenticatedSymmetricCipher> StandardCipherScheme<TScheme, TCipher>::decryptor() const
{
	return QSharedPointer<typename Scheme::Decryption>::create();
}

template <template<class> class TScheme, class TCipher>
QSharedPointer<MessageAuthenticationCode> StandardCipherScheme<TScheme, TCipher>::cmac() const
{
	return QSharedPointer<CMAC<TCipher>>::create();
}

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
PKCS8PrivateKey &RsaKeyScheme<TScheme>::privateKeyRef()
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
PKCS8PrivateKey &EccKeyScheme<TScheme>::privateKeyRef()
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

CryptoException::CryptoException(const Defaults &defaults, const QString &message, const CppException &cExcept) :
	Exception(defaults, message),
#ifndef __clang__
	_exception(cExcept)
#else
	_exception(static_cast<const CryptoPP::Exception &>(cExcept)) //NOTE clang hack. It is a CryptoPP::Exception, but for whatever reason without any type information
#endif
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

namespace {

ExtendedFactory::ExtendedFactory() :
	QPluginFactory(QStringLiteral("keystores"))
{}

bool ExtendedFactory::isAvailable(const QString &provider)
{
	if(!allKeys().contains(provider))
		return false;
	auto instance = plugin(provider);
	if(instance) {
		auto wasLoaded = isLoaded(provider);
		auto res = instance->keystoreAvailable(provider);
		if(!wasLoaded) {
			pluginObj(provider)->deleteLater();
			unload(provider);
		}
		return res;
	} else
		return false;
}

KeyStore *ExtendedFactory::createInstance(const QString &key, const Defaults &defaults, QObject *parent)
{
	auto plg = plugin(key);
	if(plg && plg->keystoreAvailable(key))
		return plg->createInstance(key, defaults, parent);
	else
		return nullptr;
}

}
