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
#include <cryptopp/pwdbased.h>

#include <qiodevicesink.h>
#include <qiodevicesource.h>
#include <qpluginfactory.h>

using namespace QtDataSync;
using namespace CryptoPP;
using std::tuple;
using std::make_tuple;
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

const CryptoController::byte CryptoController::PwPurpose(0x42);
const int CryptoController::PwRounds = 5;

const QString CryptoController::keyKeystore(QStringLiteral("keystore"));
const QString CryptoController::keySignScheme(QStringLiteral("scheme/signing"));
const QString CryptoController::keyCryptScheme(QStringLiteral("scheme/encryption"));
const QString CryptoController::keyLocalSymKey(QStringLiteral("localkey"));
const QString CryptoController::keyNextSymKey(QStringLiteral("nextkey"));
const QString CryptoController::keySymKeys(QStringLiteral("scheme/key"));
const QString CryptoController::keySymKeysTemplate(QStringLiteral("scheme/key/%1"));

const QString CryptoController::keySignKeyTemplate(QStringLiteral("%1/signing"));
const QString CryptoController::keyCryptKeyTemplate(QStringLiteral("%1/encryption"));
const QString CryptoController::keyKeyFileTemplate(QStringLiteral("key_%1.enc"));

CryptoController::CryptoController(const Defaults &defaults, QObject *parent) :
	Controller("crypto", defaults, parent),
	_keyStore(nullptr),
	_asymCrypto(nullptr),
	_loadedChiphers(),
	_localCipher(0),
	_fingerprint()
{}

QStringList CryptoController::allKeystoreKeys()
{
	return factory->allKeys();
}

QStringList CryptoController::availableKeystoreKeys()
{
	QStringList keys;
	for(auto key : factory->allKeys()) {
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
	clearKeyMaterial();
}

ClientCrypto *CryptoController::crypto() const
{
	return _asymCrypto;
}

RandomNumberGenerator &CryptoController::rng() const
{
	return _asymCrypto->rng();
}

QByteArray CryptoController::fingerprint() const
{
	return _fingerprint;
}

quint32 CryptoController::keyIndex() const
{
	return _localCipher;
}

bool CryptoController::hasKeyUpdate() const
{
	return settings()->contains(keyNextSymKey);
}

void CryptoController::acquireStore(bool existing)
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
			return;
		else {
			_keyStore->deleteLater();
			_keyStore = nullptr;
		}
	}

	_keyStore = factory->createInstance(provider, defaults(), this);
	if(!_keyStore)
		throw QtDataSync::Exception(defaults(), QStringLiteral("Keystore %1 not available").arg(provider));

	logDebug() << "Created keystore instance of type" << provider;
}

void CryptoController::loadKeyMaterial(const QUuid &deviceId)
{
	try {
		ensureStoreOpen();

		auto signScheme = settings()->value(keySignScheme).toByteArray();
		auto signKey = _keyStore->load(keySignKeyTemplate.arg(deviceId.toString()));
		if(signKey.isNull())
			throw KeyStoreException(_keyStore, QStringLiteral("Unable to load private signing key from keystore"));

		auto cryptScheme = settings()->value(keyCryptScheme).toByteArray();
		auto cryptKey = _keyStore->load(keyCryptKeyTemplate.arg(deviceId.toString()));
		if(cryptKey.isNull())
			throw KeyStoreException(_keyStore, QStringLiteral("Unable to load private encryption key from keystore"));

		_asymCrypto->load(signScheme, signKey, cryptScheme, cryptKey);
		_fingerprint = _asymCrypto->ownFingerprint();
		emit fingerprintChanged(_fingerprint);

		_localCipher = settings()->value(keyLocalSymKey, 0).toUInt();
		getInfo(_localCipher);

		logDebug().noquote() << "Loaded keys for" << deviceId
							 << "with key fingerprint:" << _fingerprint.toHex();
		closeStore();
#ifdef __clang__
	} catch(QException &e) { //prevent catching the std::exception
		throw;
#endif
	} catch(CppException &e) {
		closeStore();
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to load key material"),
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
	emit fingerprintChanged(QByteArray());
}

void CryptoController::deleteKeyMaterial(const QUuid &deviceId)
{
	//remove all saved keys
	auto keyDir = keysDir();
	if(!keyDir.removeRecursively())
		logWarning() << "Failed to completely delete old exchange keys";

	settings()->remove(keySignScheme);
	settings()->remove(keyCryptScheme);
	settings()->remove(keyLocalSymKey);
	settings()->remove(keyKeystore);

	try {
		ensureStoreOpen();
		_keyStore->remove(keySignKeyTemplate.arg(deviceId.toString()));
		_keyStore->remove(keyCryptKeyTemplate.arg(deviceId.toString()));
#ifdef __clang__
	} catch(QException &e) { //prevent catching the std::exception
		throw;
#endif
	} catch(CppException &e) {
		logWarning() << "Failed to delete private keys from keystore with error:" << e.what();
	}
	closeStore();
	//clear loaded keys
	clearKeyMaterial();
}

void CryptoController::createPrivateKeys(const QByteArray &nonce)
{
	try {
		if(_asymCrypto->rng().CanIncorporateEntropy())
			_asymCrypto->rng().IncorporateEntropy(reinterpret_cast<const byte*>(nonce.constData()), nonce.size());

		//generate private signature and encryption keys
		_asymCrypto->generate(static_cast<Setup::SignatureScheme>(defaults().property(Defaults::SignScheme).toInt()),
							  defaults().property(Defaults::SignKeyParam),
							  static_cast<Setup::EncryptionScheme>(defaults().property(Defaults::CryptScheme).toInt()),
							  defaults().property(Defaults::CryptKeyParam));
		_fingerprint = _asymCrypto->ownFingerprint();
		emit fingerprintChanged(_fingerprint);

		//create symmetric cipher and the key
		auto info = createInfo();
		_localCipher = 0;
		_loadedChiphers.insert(_localCipher, info);

		logDebug().noquote() << "Generated new keys. Public key fingerprint:"
							 << _fingerprint.toHex();
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to generate key material"),
							  e);
	}
}

void CryptoController::storePrivateKeys(const QUuid &deviceId) const
{
	try {
		ensureStoreOpen();

		settings()->setValue(keySignScheme, _asymCrypto->signatureScheme());
		_keyStore->save(keySignKeyTemplate.arg(deviceId.toString()),
								   _asymCrypto->savePrivateSignKey());

		settings()->setValue(keyCryptScheme, _asymCrypto->encryptionScheme());
		_keyStore->save(keyCryptKeyTemplate.arg(deviceId.toString()),
								   _asymCrypto->savePrivateCryptKey());

		//store with key index 0, as the initial key, that will never be managed by the server. Server will always start counting at 1
		storeCipherKey(_localCipher);
		settings()->setValue(keyLocalSymKey, _localCipher);
		logDebug() << "Stored keys for" << deviceId;
		closeStore();

		settings()->setValue(keyKeystore, _keyStore->providerName());
#ifdef __clang__
	} catch(QException &e) { //prevent catching the std::exception
		throw;
#endif
	} catch(CppException &e) {
		closeStore();
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to store key material"),
							  e);
	}
}

QByteArray CryptoController::serializeSignedMessage(const Message &message)
{
	try {
		return message.serializeSigned(_asymCrypto->privateSignKey(),
									   _asymCrypto->rng(),
									   _asymCrypto);
	} catch(CryptoPP::Exception &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to sign message"),
							  e);
	}
}

tuple<quint32, QByteArray, QByteArray> CryptoController::encryptData(const QByteArray &plain)
{
	try {
		auto info = getInfo(_localCipher);
		QByteArray salt(info.scheme->ivLength(), Qt::Uninitialized);
		_asymCrypto->rng().GenerateBlock(reinterpret_cast<byte*>(salt.data()), salt.size());

		auto cipher = encryptImpl(info, salt, plain);

		return make_tuple(_localCipher, salt, cipher);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to encrypt data for upload"),
							  e);
	}
}

QByteArray CryptoController::decryptData(quint32 keyIndex, const QByteArray &salt, const QByteArray &cipher) const
{
	try {
		auto info = getInfo(keyIndex);
		return decryptImpl(info, salt, cipher);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to decrypt downloaded data"),
							  e);
	}
}

QByteArray CryptoController::createCmac(const QByteArray &data) const
{
	return createCmac(_localCipher, data);
}

QByteArray CryptoController::createCmac(quint32 keyIndex, const QByteArray &data) const
{
	try {
		auto info = getInfo(keyIndex);
		return createCmacImpl(info, data);
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
		verifyCmacImpl(info, data, mac);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to verify CMAC"),
							  e);
	}
}

tuple<quint32, QByteArray, QByteArray> CryptoController::encryptSecretKey(AsymmetricCrypto *crypto, const X509PublicKey &pubKey) const
{
	try {
		auto info = getInfo(_localCipher);
		auto data = encryptSecretKey(_localCipher, crypto, pubKey);
		return make_tuple(_localCipher, info.scheme->name(), data);
#ifdef __clang__
	} catch(QException &e) { //prevent catching the std::exception
		throw;
#endif
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to encrypt exchange key for peer"),
							  e);
	}
}

QByteArray CryptoController::encryptSecretKey(quint32 keyIndex, AsymmetricCrypto *crypto, const X509PublicKey &pubKey) const
{
	try {
		auto info = getInfo(keyIndex);
		auto data = crypto->encrypt(pubKey,
									_asymCrypto->rng(),
									QByteArray::fromRawData(reinterpret_cast<const char*>(info.key.data()), static_cast<int>(info.key.size())));
		return data;
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to encrypt exchange key for peer"),
							  e);
	}
}

void CryptoController::decryptSecretKey(quint32 keyIndex, const QByteArray &scheme, const QByteArray &data, bool grantInit)
{
	try {
		auto key = _asymCrypto->decrypt(data);

		CipherInfo info;
		createScheme(scheme, info.scheme);
		info.key.Assign(reinterpret_cast<const byte*>(key.constData()), key.size());

		_loadedChiphers.insert(keyIndex, info);
		if(grantInit) {//only set key index. Storing is done a step later for granting
			_localCipher = keyIndex;
			logDebug() << "Loaded initial exchange key with index" << _localCipher;
		} else {
			storeCipherKey(keyIndex);
			if(keyIndex > _localCipher) {
				_localCipher = keyIndex;
				settings()->setValue(keyLocalSymKey, _localCipher);
				logInfo() << "Imported exchange key with index" << _localCipher
						  << "and updated local key to this index";
				cleanCiphers();
			} else
				logDebug() << "Imported old exchange key with index" << keyIndex;
		}
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to exchange secret key from peer"),
							  e);
	}
}

QByteArray CryptoController::generateEncryptionKeyCmac() const
{
	return generateEncryptionKeyCmac(_localCipher);
}

QByteArray CryptoController::generateEncryptionKeyCmac(quint32 keyIndex) const
{
	try {
		QByteArray message = _asymCrypto->encryptionScheme() +
							 _asymCrypto->writeCryptKey();
		auto info = getInfo(keyIndex);
		return createCmacImpl(info, message);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to create CMAC for private encryption key"),
							  e);
	}
}

void CryptoController::verifyEncryptionKeyCmac(AsymmetricCrypto *crypto, const X509PublicKey &pubKey, const QByteArray &cmac) const
{
	try {
		QByteArray message = crypto->encryptionScheme() +
							 crypto->writeKey(pubKey);
		auto info = getInfo(_localCipher);
		verifyCmacImpl(info, message, cmac);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to verify CMAC for private encryption key"),
							  e);
	}
}

tuple<quint32, QByteArray> CryptoController::generateNextKey()
{
	try {
		auto keyIndex = _localCipher + 1;
		auto nIndex = settings()->value(keyNextSymKey);
		CipherInfo info;
		if(nIndex.isValid() && nIndex.toUInt() == keyIndex)
			info = getInfo(keyIndex);
		else {
			info = createInfo();
			_loadedChiphers.insert(keyIndex, info);
			storeCipherKey(keyIndex);
			settings()->setValue(keyNextSymKey, keyIndex);
			//not set as active key yet
			logDebug().noquote() << "Generated new exchange key with index" << keyIndex;
		}
		return make_tuple(keyIndex, info.scheme->name());
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to generate new exchange key"),
							  e);
	}
}

void CryptoController::activateNextKey(quint32 keyIndex)
{
	try {
		if(_localCipher + 1 != keyIndex) {
			logWarning() << "Unexpected keychange index, is not current + 1";
			return;
		}

		auto nIndex = settings()->value(keyNextSymKey).toUInt();
		if(keyIndex != nIndex) {
			logWarning() << "Unexpected keychange index, is not the currently store key proposal";
			return;
		}

		getInfo(keyIndex); // just to make shure the key is actually available and can be loaded

		_localCipher = keyIndex;
		settings()->setValue(keyLocalSymKey, _localCipher);
		settings()->remove(keyNextSymKey);
		logDebug() << "Stored and applied new exchange key";
		cleanCiphers();
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to save new exchange key"),
							  e);
	}
}

tuple<QByteArray, QByteArray, SecByteBlock> CryptoController::generateExportKey(const QString &password) const
{
	try {
		//load the algorithm
		CipherInfo info;
		createScheme(static_cast<Setup::CipherScheme>(defaults().property(Defaults::SymScheme).toInt()), info.scheme);
		info.key.CleanNew(info.scheme->defaultKeyLength());

		QByteArray salt;
		if(password.isNull()) {
			//generate random key
			_asymCrypto->rng().GenerateBlock(info.key.data(), info.key.size());
		} else {
			auto pw = password.toUtf8();

			//create a salt
			salt.resize(info.scheme->ivLength());
			_asymCrypto->rng().GenerateBlock(reinterpret_cast<byte*>(salt.data()), salt.size());

			//generate the key
			PKCS5_PBKDF2_HMAC<SHA3_256> keydev;
			keydev.DeriveKey(info.key.data(), info.key.size(),
							 PwPurpose, reinterpret_cast<const byte*>(pw.constData()), pw.size(),
							 reinterpret_cast<const byte*>(salt.constData()), salt.size(), PwRounds);
		}

		return make_tuple(info.scheme->name(), salt, info.key);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to generate key from password"),
							  e);
	}
}

SecByteBlock CryptoController::recoverExportKey(const QByteArray &scheme, const QByteArray &salt, const QString &password) const
{
	try {
		auto pw = password.toUtf8();

		//load the algorithm
		CipherInfo info;
		createScheme(scheme, info.scheme);
		info.key.CleanNew(info.scheme->defaultKeyLength());

		//generate the key
		PKCS5_PBKDF2_HMAC<SHA3_256> keydev;
		keydev.DeriveKey(info.key.data(), info.key.size(),
						 PwPurpose, reinterpret_cast<const byte*>(pw.constData()), pw.size(),
						 reinterpret_cast<const byte*>(salt.constData()), salt.size(), PwRounds);

		return info.key;
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to recover key from password"),
							  e);
	}
}

QByteArray CryptoController::createExportCmac(const QByteArray &scheme, const SecByteBlock &key, const QByteArray &data) const
{
	try {
		CipherInfo info;
		createScheme(scheme, info.scheme);
		info.key = key;
		return createCmacImpl(info, data);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to generate cmac for export data"),
							  e);
	}
}

QByteArray CryptoController::createExportCmacForCrypto(const QByteArray &scheme, const SecByteBlock &key) const
{
	try {
		QByteArray trustMessage = _asymCrypto->signatureScheme() +
								  _asymCrypto->writeSignKey() +
								  _asymCrypto->encryptionScheme() +
								  _asymCrypto->writeCryptKey();
		CipherInfo info;
		createScheme(scheme, info.scheme);
		info.key = key;
		return createCmacImpl(info, trustMessage);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to generate cmac for crypto keys"),
							  e);
	}
}

void CryptoController::verifyImportCmac(const QByteArray &scheme, const SecByteBlock &key, const QByteArray &data, const QByteArray &mac) const
{
	try {
		CipherInfo info;
		createScheme(scheme, info.scheme);
		info.key = key;
		return verifyCmacImpl(info, data, mac);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to verify cmac for import data"),
							  e);
	}
}

void CryptoController::verifyImportCmacForCrypto(const QByteArray &scheme, const SecByteBlock &key, AsymmetricCryptoInfo *cryptoInfo, const QByteArray &mac) const
{
	try {
		QByteArray trustMessage = cryptoInfo->signatureScheme() +
								  cryptoInfo->writeKey(cryptoInfo->signatureKey()) +
								  cryptoInfo->encryptionScheme() +
								  cryptoInfo->writeKey(cryptoInfo->encryptionKey());
		CipherInfo info;
		createScheme(scheme, info.scheme);
		info.key = key;
		return verifyCmacImpl(info, trustMessage, mac);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to verify cmac for crypto keys"),
							  e);
	}
}

QByteArray CryptoController::exportEncrypt(const QByteArray &scheme, const QByteArray &salt, const SecByteBlock &key, const QByteArray &data) const
{
	try {
		CipherInfo info;
		createScheme(scheme, info.scheme);
		info.key = key;
		return encryptImpl(info, salt, data);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to encrypt export data"),
							  e);
	}
}

QByteArray CryptoController::importDecrypt(const QByteArray &scheme, const QByteArray &salt, const SecByteBlock &key, const QByteArray &data) const
{
	try {
		CipherInfo info;
		createScheme(scheme, info.scheme);
		info.key = key;
		return decryptImpl(info, salt, data);
	} catch(CppException &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to decrypt import data"),
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
		if(!_keyStore->isOpen()) {
			_keyStore->openStore();
			logDebug() << "Opened keystore to access keys";
		}
	} else
		throw CryptoPP::Exception(CryptoPP::Exception::NOT_IMPLEMENTED, "No keystore available");
}

void CryptoController::closeStore() const
{
	try {
		if(_keyStore && _keyStore->isOpen()) {
			_keyStore->closeStore();
			logDebug() << "Closed keystore access";
		}
	} catch(QException &e) {
		logCritical() << "Failed to close keystore with error:" << e.what();
	}
}

QDir CryptoController::keysDir() const
{
	auto keyDirName = QStringLiteral("keys");
	auto keyDir = defaults().storageDir();
	if(!keyDir.mkpath(keyDirName) || !keyDir.cd(keyDirName))
		throw CryptoPP::Exception(CryptoPP::Exception::IO_ERROR, "Failed to create keys directory");
	return keyDir;
}

CryptoController::CipherInfo CryptoController::createInfo() const
{
	CipherInfo info;
	createScheme(static_cast<Setup::CipherScheme>(defaults().property(Defaults::SymScheme).toInt()),
				 info.scheme);
	auto keySize = defaults().property(Defaults::SymKeyParam).toUInt();
	if(keySize == 0)
		keySize = info.scheme->defaultKeyLength();
	else
		keySize = info.scheme->toKeyLength(keySize);
	info.key.CleanNew(keySize);
	_asymCrypto->rng().GenerateBlock(info.key.data(), info.key.size());
	return info;
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
		info.key.Assign(reinterpret_cast<const byte*>(key.constData()), key.size());
		memset(key.data(), 0, key.size());

		//test if the key is of valid length
		if(info.key.size() != info.scheme->toKeyLength(static_cast<quint32>(info.key.size())))
			throw CryptoPP::Exception(CryptoPP::Exception::OTHER_ERROR, "Key size is not valid for cipher scheme " + info.scheme->name().toStdString());
		_loadedChiphers.insert(keyIndex, info);
		logDebug() << "Loaded stored exchange key for index" << keyIndex;
	}

	return _loadedChiphers[keyIndex];
}

void CryptoController::storeCipherKey(quint32 keyIndex) const
{
	auto keyDir = keysDir();
	auto info = getInfo(keyIndex);
	auto encData = _asymCrypto->encrypt(_asymCrypto->cryptKey(),
										QByteArray::fromRawData(reinterpret_cast<const char*>(info.key.data()),
																static_cast<int>(info.key.size())));
	QFile keyFile(keyDir.absoluteFilePath(keyKeyFileTemplate.arg(keyIndex)));
	if(!keyFile.open(QIODevice::WriteOnly))
		throw CryptoPP::Exception(CryptoPP::Exception::IO_ERROR, keyFile.errorString().toStdString());
	keyFile.write(encData);
	keyFile.close();

	settings()->setValue(keySymKeysTemplate.arg(keyIndex), info.scheme->name());
}

void CryptoController::cleanCiphers() const
{
	auto keyDir = keysDir();

	settings()->beginGroup(keySymKeys);
	auto keys = settings()->childKeys();
	settings()->endGroup();

	for(auto key : keys) {
		auto ok = false;
		auto keyIndex = key.toUInt(&ok);
		if(!ok)
			continue;

		if((keyIndex + 5) < _localCipher) {
			settings()->remove(keySymKeysTemplate.arg(keyIndex));
			if(!keyDir.remove(keyKeyFileTemplate.arg(keyIndex)))
				logWarning() << "Failed to delete file of cleared key with index" << keyIndex;
			_loadedChiphers.remove(keyIndex);
		}
	}
}

QByteArray CryptoController::createCmacImpl(const CryptoController::CipherInfo &info, const QByteArray &data) const
{
	auto cmac = info.scheme->cmac();
	cmac->SetKey(info.key.data(), info.key.size());

	QByteArray mac;
	QByteArraySource (data, true,
		new HashFilter(*cmac,
			new QByteArraySink(mac)
		) // HashFilter
	); // QByteArraySource

	return mac;
}

void CryptoController::verifyCmacImpl(const CryptoController::CipherInfo &info, const QByteArray &data, const QByteArray &mac) const
{
	auto cmac = info.scheme->cmac();
	cmac->SetKey(info.key.data(), info.key.size());

	QByteArraySource (data + mac, true,
		new HashVerificationFilter(*cmac, nullptr, HashVerificationFilter::THROW_EXCEPTION | HashVerificationFilter::HASH_AT_END) // HashFilter
	); // QByteArraySource
}

QByteArray CryptoController::encryptImpl(const CryptoController::CipherInfo &info, const QByteArray &salt, const QByteArray &plain) const
{
	auto enc = info.scheme->encryptor();
	enc->SetKeyWithIV(info.key.data(), info.key.size(),
					  reinterpret_cast<const byte*>(salt.constData()), salt.size());

	QByteArray cipher;
	QByteArraySource(plain, true,
		new AuthenticatedEncryptionFilter(*enc,
			new QByteArraySink(cipher)
		) // AuthenticatedEncryptionFilter
	); // QByteArraySource
	return cipher;
}

QByteArray CryptoController::decryptImpl(const CryptoController::CipherInfo &info, const QByteArray &salt, const QByteArray &cipher) const
{
	auto dec = info.scheme->decryptor();
	dec->SetKeyWithIV(info.key.data(), info.key.size(),
					  reinterpret_cast<const byte*>(salt.constData()), salt.size());

	QByteArray plain;
	QByteArraySource(cipher, true,
		new AuthenticatedDecryptionFilter(*dec,
			new QByteArraySink(plain)
		) // AuthenticatedDecryptionFilter
	); // QByteArraySource
	return plain;
}

// ------------- ClientCrypto Implementation -------------

ClientCrypto::ClientCrypto(QObject *parent) :
	AsymmetricCrypto(parent),
#ifdef QT_NO_DEBUG
	_rng(true, 32),
#else //fast rng seeding for debug builds
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

QByteArray ClientCrypto::ownFingerprint() const
{
	return fingerprint(signKey(), cryptKey());
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
#if CRYPTOPP_VERSION >= 600
	else if(stdStr == EciesScheme::StaticAlgorithmName())
		_cryptKey.reset(new EccKeyScheme<EciesScheme>());
#endif
	else
		throw CryptoPP::Exception(CryptoPP::Exception::NOT_IMPLEMENTED, "Encryption Scheme \"" + stdStr + "\" not supported");
}

void ClientCrypto::setEncryptionKey(Setup::EncryptionScheme scheme)
{
	switch (scheme) {
	case Setup::ECIES_ECP_SHA3_512:
#if CRYPTOPP_VERSION >= 600
		setEncryptionKey(QByteArray::fromStdString(EciesScheme::StaticAlgorithmName()));
		break;
#else
		qWarning() << "Encryption scheme" << scheme
				   << "can only be used with cryptopp > 6.0. Falling back to" << Setup::RSA_OAEP_SHA3_512;
		Q_FALLTHROUGH();
#endif
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
	return static_cast<quint32>(TCipher::StaticGetValidKeyLength(length));
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
	auto curve = ClientCrypto::curveId(static_cast<Setup::EllipticCurve>(keyParam.toInt()));

	//special hack: save and load again for consistency (needed for fingerprint)
	typename TScheme::PrivateKey tmpKey;
	tmpKey.Initialize(rng, curve);
	QByteArray buffer;
	QByteArraySink sink(buffer);
	tmpKey.Save(sink);
	QByteArraySource source(buffer, true);
	_key.Load(source);
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
			.arg(type());
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
