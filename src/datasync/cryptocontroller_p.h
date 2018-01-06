#ifndef CRYPTOCONTROLLER_P_H
#define CRYPTOCONTROLLER_P_H

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
	class Q_DATASYNC_EXPORT CipherScheme
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

	ClientCrypto *crypto() const;
	CryptoPP::RandomNumberGenerator &rng() const;
	QByteArray fingerprint() const;
	quint32 keyIndex() const;

	std::tuple<quint32, QByteArray, QByteArray> encryptSecretKey(AsymmetricCrypto *crypto, const CryptoPP::X509PublicKey &pubKey) const; //(keyIndex, scheme, data)
	void decryptSecretKey(quint32 keyIndex, const QByteArray &scheme, const QByteArray &data, bool grantInit);
	QByteArray generateCryptoKeyCmac() const;
	void verifyCryptoKeyCmac(quint32 oldIndex, AsymmetricCrypto *crypto, const CryptoPP::X509PublicKey &pubKey, const QByteArray &cmac) const;
	quint32 generateNextKey();
	void saveNextKey(quint32 keyIndex);

	bool acquireStore(bool existing);
	void loadKeyMaterial(const QUuid &deviceId);
	void clearKeyMaterial();
	void deleteKeyMaterial(const QUuid &deviceId);

	void createPrivateKeys(const QByteArray &nonce);
	void storePrivateKeys(const QUuid &deviceId) const;

	template <typename TMessage>
	QByteArray serializeSignedMessage(const TMessage &message);

	std::tuple<quint32, QByteArray, QByteArray> encrypt(const QByteArray &data); //(keyIndex, salt, data)
	QByteArray decrypt(quint32 keyIndex, const QByteArray &salt, const QByteArray &cipher) const;

	std::tuple<quint32, QByteArray> createCmac(const QByteArray &data) const; //(keyIndex, cmac)
	QByteArray createCmac(quint32 keyIndex, const QByteArray &data) const; //(keyIndex, cmac)
	void verifyCmac(quint32 keyIndex, const QByteArray &data, const QByteArray &mac) const;

	//exchange stuff
	std::tuple<QByteArray, QByteArray, CryptoPP::SecByteBlock> generateExportKey(const QString &password) const; //(scheme, salt, key)
	CryptoPP::SecByteBlock recoverExportKey(const QByteArray &scheme, const QByteArray &salt, const QString &password) const;
	QByteArray createExportCmac(const QByteArray &scheme, const CryptoPP::SecByteBlock &key, const QByteArray &data) const;
	void verifyImportCmac(const QByteArray &scheme, const CryptoPP::SecByteBlock &key, const QByteArray &data, const QByteArray &mac) const;
	QByteArray exportEncrypt(const QByteArray &scheme, const QByteArray &salt, const CryptoPP::SecByteBlock &key, const QByteArray &data) const;
	QByteArray importDecrypt(const QByteArray &scheme, const QByteArray &salt, const CryptoPP::SecByteBlock &key, const QByteArray &data) const;

Q_SIGNALS:
	void fingerprintChanged(const QByteArray &fingerprint);

private:
	struct Q_DATASYNC_EXPORT CipherInfo {
		QSharedPointer<CipherScheme> scheme;
		CryptoPP::SecByteBlock key;
	};

	static const byte PwPurpose;
	static const int PwRounds;

	static const QString keyKeystore;
	static const QString keySignScheme;
	static const QString keyCryptScheme;
	static const QString keyLocalSymKey;
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
	void storeCipherKey(quint32 keyIndex) const;
	void cleanCiphers() const;

	CipherInfo createCipher() const;
	const CipherInfo &getInfo(quint32 keyIndex) const;
	QDir keysDir() const;
	QByteArray genCmac(const CipherInfo &info, const QByteArray &data) const;
	void verCmac(const CipherInfo &info, const QByteArray &data, const QByteArray &mac) const;
	QByteArray symEncrypt(const CipherInfo &info, const QByteArray &salt, const QByteArray &plain) const;
	QByteArray symDecrypt(const CipherInfo &info, const QByteArray &salt, const QByteArray &cipher) const;
};

class Q_DATASYNC_EXPORT ClientCrypto : public AsymmetricCrypto
{
	Q_OBJECT

public:
	class Q_DATASYNC_EXPORT KeyScheme
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
		return encrypt(*(key.data()), message);
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

// ------------- Generic Implementation -------------

template<typename TMessage>
QByteArray CryptoController::serializeSignedMessage(const TMessage &message)
{
	try {
		return QtDataSync::serializeSignedMessage<TMessage>(message,
															_asymCrypto->privateSignKey(),
															_asymCrypto->rng(),
															_asymCrypto);
	} catch(CryptoPP::Exception &e) {
		throw CryptoException(defaults(),
							  QStringLiteral("Failed to sign message"),
							  e);
	}
}
}

#endif // CRYPTOCONTROLLER_P_H
