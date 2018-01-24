#ifndef QTDATASYNC_CRYPTOCONTROLLER_P_H
#define QTDATASYNC_CRYPTOCONTROLLER_P_H

#include <tuple>

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QPointer>

#include <cryptopp/config.h>
#ifndef OS_RNG_AVAILABLE
#ifdef QTDATASYNC_OSRNG_OVERWRITE
#define OS_RNG_AVAILABLE
#endif
#endif
#include <cryptopp/osrng.h>
#include <cryptopp/oids.h>

#include "qtdatasync_global.h"
#include "defaults.h"
#include "keystore.h"
#include "controller_p.h"

#include "message_p.h"
#include "asymmetriccrypto_p.h"

namespace QtDataSync {

class ClientCrypto;
class SymmetricCrypto;

class Q_DATASYNC_EXPORT CryptoController : public Controller
{
	Q_OBJECT

	Q_PROPERTY(QByteArray fingerprint READ fingerprint NOTIFY fingerprintChanged)

public:
#if CRYPTOPP_VERSION >= 600
	using byte = CryptoPP::byte;
#else
	using byte = ::byte;
#endif

	//not exported, only public to be reimplemented
	class CipherScheme
	{
	public:
		inline virtual ~CipherScheme() = default;

		virtual QByteArray name() const = 0;
		virtual quint32 defaultKeyLength() const = 0;
		virtual quint32 ivLength() const = 0;
		virtual quint32 toKeyLength(quint32 length) const = 0;
		virtual QSharedPointer<CryptoPP::AuthenticatedSymmetricCipher> encryptor() const = 0;
		virtual QSharedPointer<CryptoPP::AuthenticatedSymmetricCipher> decryptor() const = 0;
		virtual QSharedPointer<CryptoPP::MessageAuthenticationCode> cmac() const = 0;
	};

	explicit CryptoController(const Defaults &defaults, QObject *parent = nullptr);

	static QStringList allKeystoreKeys();
	static QStringList availableKeystoreKeys();
	static bool keystoreAvailable(const QString &provider);

	void initialize(const QVariantHash &params) final;
	void finalize() final;

	//status access
	ClientCrypto *crypto() const;
	CryptoPP::RandomNumberGenerator &rng() const;
	QByteArray fingerprint() const;
	quint32 keyIndex() const;
	bool hasKeyUpdate() const;

	//load, clear, remove key material
	void acquireStore(bool existing);
	void loadKeyMaterial(const QUuid &deviceId);
	void clearKeyMaterial();
	void deleteKeyMaterial(const QUuid &deviceId);

	//create and store new keys
	void createPrivateKeys(const QByteArray &nonce);
	void storePrivateKeys(const QUuid &deviceId) const;

	//wrapper to sign a message
	QByteArray serializeSignedMessage(const Message &message);

	//used for transport encryption of actual data
	std::tuple<quint32, QByteArray, QByteArray> encryptData(const QByteArray &data); //(keyIndex, salt, data)
	QByteArray decryptData(quint32 keyIndex, const QByteArray &salt, const QByteArray &cipher) const;

	// cmac generation for verification of key updates etc.
	QByteArray createCmac(const QByteArray &data) const;
	QByteArray createCmac(quint32 keyIndex, const QByteArray &data) const; //(keyIndex, cmac)
	void verifyCmac(quint32 keyIndex, const QByteArray &data, const QByteArray &mac) const;

	// secret key related, for exchanging the secret key with other devices
	std::tuple<quint32, QByteArray, QByteArray> encryptSecretKey(AsymmetricCrypto *crypto, const CryptoPP::X509PublicKey &pubKey) const; //(keyIndex, scheme, data)
	QByteArray encryptSecretKey(quint32 keyIndex, AsymmetricCrypto *crypto, const CryptoPP::X509PublicKey &pubKey) const;
	void decryptSecretKey(quint32 keyIndex, const QByteArray &scheme, const QByteArray &data, bool grantInit);
	QByteArray generateEncryptionKeyCmac() const;
	QByteArray generateEncryptionKeyCmac(quint32 keyIndex) const;
	void verifyEncryptionKeyCmac(AsymmetricCrypto *crypto, const CryptoPP::X509PublicKey &pubKey, const QByteArray &cmac) const;
	std::tuple<quint32, QByteArray> generateNextKey(); //(keyIndex, scheme)
	void activateNextKey(quint32 keyIndex);

	//export and import key methods for exchange security
	std::tuple<QByteArray, QByteArray, CryptoPP::SecByteBlock> generateExportKey(const QString &password) const; //(scheme, salt, key)
	CryptoPP::SecByteBlock recoverExportKey(const QByteArray &scheme,
											const QByteArray &salt,
											const QString &password) const;
	QByteArray createExportCmac(const QByteArray &scheme,
								const CryptoPP::SecByteBlock &key,
								const QByteArray &data) const;
	QByteArray createExportCmacForCrypto(const QByteArray &scheme,
										 const CryptoPP::SecByteBlock &key) const;
	void verifyImportCmac(const QByteArray &scheme,
						  const CryptoPP::SecByteBlock &key,
						  const QByteArray &data,
						  const QByteArray &mac) const;
	void verifyImportCmacForCrypto(const QByteArray &scheme,
								   const CryptoPP::SecByteBlock &key,
								   AsymmetricCryptoInfo *cryptoInfo,
								   const QByteArray &mac) const;
	QByteArray exportEncrypt(const QByteArray &scheme,
							 const QByteArray &salt,
							 const CryptoPP::SecByteBlock &key,
							 const QByteArray &data) const;
	QByteArray importDecrypt(const QByteArray &scheme,
							 const QByteArray &salt,
							 const CryptoPP::SecByteBlock &key,
							 const QByteArray &data) const;

Q_SIGNALS:
	void fingerprintChanged(const QByteArray &fingerprint);

private:
	//dont export private classes
	struct CipherInfo {
		QSharedPointer<CipherScheme> scheme;
		CryptoPP::SecByteBlock key;
	};

	static const byte PwPurpose;
	static const int PwRounds;

	static const QString keyKeystore;
	static const QString keySignScheme;
	static const QString keyCryptScheme;
	static const QString keyLocalSymKey;
	static const QString keyNextSymKey;
	static const QString keySymKeys;
	static const QString keySymKeysTemplate;

	static const QString keySignKeyTemplate;
	static const QString keyCryptKeyTemplate;
	static const QString keyKeyFileTemplate;

	QPointer<KeyStore> _keyStore;
	ClientCrypto *_asymCrypto;
	mutable QHash<quint32, CipherInfo> _loadedChiphers;
	quint32 _localCipher;

	QByteArray _fingerprint;

	static void createScheme(const QByteArray &name, QSharedPointer<CipherScheme> &ptr);
	static void createScheme(Setup::CipherScheme scheme, QSharedPointer<CipherScheme> &ptr);

	void ensureStoreOpen() const;
	void closeStore() const;

	QDir keysDir() const;
	CipherInfo createInfo() const;
	const CipherInfo &getInfo(quint32 keyIndex) const;
	void storeCipherKey(quint32 keyIndex) const;
	void cleanCiphers() const;

	QByteArray createCmacImpl(const CipherInfo &info, const QByteArray &data) const;
	void verifyCmacImpl(const CipherInfo &info, const QByteArray &data, const QByteArray &mac) const;
	QByteArray encryptImpl(const CipherInfo &info, const QByteArray &salt, const QByteArray &plain) const;
	QByteArray decryptImpl(const CipherInfo &info, const QByteArray &salt, const QByteArray &cipher) const;
};

class Q_DATASYNC_EXPORT ClientCrypto : public AsymmetricCrypto
{
	Q_OBJECT

public:
	//not exported, only public to be reimplemented
	class KeyScheme
	{
	public:
		inline virtual ~KeyScheme() = default;

		virtual QByteArray name() const = 0;
		virtual void createPrivateKey(CryptoPP::RandomNumberGenerator &rng, const QVariant &keyParam) = 0;
		virtual CryptoPP::PKCS8PrivateKey &privateKeyRef() = 0;
		virtual QSharedPointer<CryptoPP::X509PublicKey> createPublicKey() const = 0;
	};

	ClientCrypto(QObject *parent = nullptr);

	void generate(Setup::SignatureScheme signScheme,
				  const QVariant &signKeyParam,
				  Setup::EncryptionScheme cryptScheme,
				  const QVariant &cryptKeyParam);

	void load(const QByteArray &signScheme,
			  const QByteArray &signKey,
			  const QByteArray &cryptScheme,
			  const QByteArray &cryptKey);

	void reset();

	CryptoPP::RandomNumberGenerator &rng();

	QByteArray ownFingerprint() const;

	QSharedPointer<CryptoPP::X509PublicKey> readKey(bool signKey, const QByteArray &data);

	QSharedPointer<CryptoPP::X509PublicKey> signKey() const;
	QByteArray writeSignKey() const;
	QSharedPointer<CryptoPP::X509PublicKey> cryptKey() const;
	QByteArray writeCryptKey() const;

	const CryptoPP::PKCS8PrivateKey &privateSignKey() const;
	QByteArray savePrivateSignKey() const;
	const CryptoPP::PKCS8PrivateKey &privateCryptKey() const;
	QByteArray savePrivateCryptKey() const;

	QByteArray sign(const QByteArray &message);

	QByteArray encrypt(const CryptoPP::X509PublicKey &key, const QByteArray &message);
	inline QByteArray encrypt(const QSharedPointer<CryptoPP::X509PublicKey> &key, const QByteArray &message) {
		return encrypt(*key, message);
	}
	QByteArray decrypt(const QByteArray &message);

	static CryptoPP::OID curveId(Setup::EllipticCurve curve);

private:
	CryptoPP::AutoSeededRandomPool _rng;
	QScopedPointer<KeyScheme> _signKey;
	QScopedPointer<KeyScheme> _cryptKey;

	void setSignatureKey(const QByteArray &name);
	void setSignatureKey(Setup::SignatureScheme scheme);
	void setEncryptionKey(const QByteArray &name);
	void setEncryptionKey(Setup::EncryptionScheme scheme);

	void loadKey(CryptoPP::PKCS8PrivateKey &key, const QByteArray &data) const;
	QByteArray saveKey(const CryptoPP::PKCS8PrivateKey &key) const;
};

class Q_DATASYNC_EXPORT CryptoException : public Exception
{
public:
#ifndef __clang__
	using CppException = CryptoPP::Exception;
#else
	using CppException = std::exception;
#endif

	CryptoException(const Defaults &defaults,
					const QString &message,
					const CppException &cExcept);

	CryptoPP::Exception cryptoPPException() const;
	QString error() const;
	CryptoPP::Exception::ErrorType type() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	CryptoException(const CryptoException * const other);
	CryptoPP::Exception _exception;
};

}

#endif // QTDATASYNC_CRYPTOCONTROLLER_P_H
