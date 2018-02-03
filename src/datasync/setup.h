#ifndef QTDATASYNC_SETUP_H
#define QTDATASYNC_SETUP_H

#include <functional>
#include <ratio>

#include <QtCore/qobject.h>
#include <QtCore/qlogging.h>
#include <QtCore/qurl.h>
class QLockFile;

#include <QtNetwork/qsslconfiguration.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"
#include "QtDataSync/remoteconfig.h"

class QJsonSerializer;

namespace QtDataSync {

//! @private
template<typename TRatio>
Q_DECL_CONSTEXPR inline int ratioBytes(const intmax_t &value);
//! Interprets value as kilobytes and returns it converted to bytes
Q_DECL_CONSTEXPR inline int KB(const intmax_t &value);
//! Interprets value as megabytes and returns it converted to bytes
Q_DECL_CONSTEXPR inline int MB(const intmax_t &value);
//! Interprets value as gigabytes and returns it converted to bytes
Q_DECL_CONSTEXPR inline int GB(const intmax_t &value);

#if __cplusplus >= 201402L
//! Sub namespace that defines the literals for MB, KB, GB
namespace literals {

//! literal for kilobytes
Q_DECL_CONSTEXPR inline int operator "" _kb(unsigned long long x) {
	return KB(static_cast<int>(x));
}

//! literal for megabytes
Q_DECL_CONSTEXPR inline int operator "" _mb(unsigned long long x) {
	return MB(static_cast<int>(x));
}

//! literal for gigabytes
Q_DECL_CONSTEXPR inline int operator "" _gb(unsigned long long x) {
	return GB(static_cast<int>(x));
}

}
#endif

class ConflictResolver;

class SetupPrivate;
//! The class to setup and create datasync instances
class Q_DATASYNC_EXPORT Setup
{
	Q_GADGET
	Q_DISABLE_COPY(Setup)

	//! The local storage directoy used by the instance
	Q_PROPERTY(QString localDir READ localDir WRITE setLocalDir RESET resetLocalDir)
	//! The url to be used to host the remote object sources, and to connect to to acquire the replicas
	Q_PROPERTY(QUrl remoteObjectHost READ remoteObjectHost WRITE setRemoteObjectHost RESET resetRemoteObjectHost)
	//! The serializer to be used to serialize and deserialize data to and from the store
	Q_PROPERTY(QJsonSerializer* serializer READ serializer WRITE setSerializer RESET resetSerializer)
	//! An optional conflict resolver to handle merge conflicts
	Q_PROPERTY(ConflictResolver* conflictResolver READ conflictResolver WRITE setConflictResolver RESET resetConflictResolver)
	//! An alternative handler for fatal errors
	Q_PROPERTY(FatalErrorHandler fatalErrorHandler READ fatalErrorHandler WRITE setFatalErrorHandler RESET resetFatalErrorHandler)
	//! The size of the internal database cache, in bytes
	Q_PROPERTY(int cacheSize READ cacheSize WRITE setCacheSize RESET resetCacheSize)
	//! Specify whether deleted datasets should persist
	Q_PROPERTY(bool persistDeletedVersion READ persistDeletedVersion WRITE setPersistDeletedVersion RESET resetPersistDeletedVersion)
	//! The policiy for how to handle conflicts
	Q_PROPERTY(SyncPolicy syncPolicy READ syncPolicy WRITE setSyncPolicy RESET resetSyncPolicy)
	//! The ssl configuration to be used to connect to the remote
	Q_PROPERTY(QSslConfiguration sslConfiguration READ sslConfiguration WRITE setSslConfiguration RESET resetSslConfiguration)
	//! The configuration to be used to connect to the remote
	Q_PROPERTY(RemoteConfig remoteConfiguration READ remoteConfiguration WRITE setRemoteConfiguration RESET resetRemoteConfiguration)
	//! The name of the preferred keystore provider
	Q_PROPERTY(QString keyStoreProvider READ keyStoreProvider WRITE setKeyStoreProvider RESET resetKeyStoreProvider)
	//! The algorithmic scheme to be used for new signature keys
	Q_PROPERTY(SignatureScheme signatureScheme READ signatureScheme WRITE setSignatureScheme RESET resetSignatureScheme)
	//! The generation parameter for the signature key
	Q_PROPERTY(QVariant signatureKeyParam READ signatureKeyParam WRITE setSignatureKeyParam RESET resetSignatureKeyParam)
	//! The algorithmic scheme to be used for new encryption keys
	Q_PROPERTY(EncryptionScheme encryptionScheme READ encryptionScheme WRITE setEncryptionScheme RESET resetEncryptionScheme)
	//! The generation parameter for the encryption key
	Q_PROPERTY(QVariant encryptionKeyParam READ encryptionKeyParam WRITE setEncryptionKeyParam RESET resetEncryptionKeyParam)
	//! The algorithmic scheme to be used for new secret exchange keys (which are symmetric)
	Q_PROPERTY(CipherScheme cipherScheme READ cipherScheme WRITE setCipherScheme RESET resetCipherScheme)
	//! The size in bytes for the secret exchange key (which is symmetric)
	Q_PROPERTY(qint32 cipherKeySize READ cipherKeySize WRITE setCipherKeySize RESET resetCipherKeySize)

public:
	//! Typedef of an error handler function. See Setup::fatalErrorHandler
	typedef std::function<void (QString, QString, const QMessageLogContext &)> FatalErrorHandler;

	//! Defines the possible policies on how to treat merge conflicts between change and delete
	enum SyncPolicy {
		PreferChanged, //!< Keep the changed entry
		PreferDeleted //! < Discard the change and keep the entry deleted
	};
	Q_ENUM(SyncPolicy)

	//! The signature schemes supported for Setup::signatureScheme
	enum SignatureScheme {
		RSA_PSS_SHA3_512, //!< RSA in PSS mode with Sha3 hash of 512 bits
		ECDSA_ECP_SHA3_512, //!< ECDSA on prime curves with Sha3 hash of 512 bits
		ECNR_ECP_SHA3_512 //!< ECNR on prime curves with Sha3 hash of 512 bits
	};
	Q_ENUM(SignatureScheme)

	//! The encryption schemes supported for Setup::encryptionScheme
	enum EncryptionScheme {
		RSA_OAEP_SHA3_512, //!< RSA in OAEP mode with Sha3 hash of 512 bits
		ECIES_ECP_SHA3_512 //!< ECIES on prime curves with Sha3 hash of 512 bits (Requires at least crypto++ 6.0)
	};
	Q_ENUM(EncryptionScheme)

	//! The symmetric cipher schemes supported for Setup::cipherScheme
	enum CipherScheme {
		AES_EAX, //!< AES operating in EAX authenticated encryption mode
		AES_GCM, //!< AES operating in GCM authenticated encryption mode
		TWOFISH_EAX, //!< Twofish operating in EAX authenticated encryption mode
		TWOFISH_GCM, //!< Twofish operating in GCM authenticated encryption mode
		SERPENT_EAX, //!< Serpent operating in EAX authenticated encryption mode
		SERPENT_GCM, //!< Serpent operating in GCM authenticated encryption mode
		IDEA_EAX, //!< IDEA operating in EAX authenticated encryption mode
	};
	Q_ENUM(CipherScheme)

	//! Elliptic curves supported as key parameter for Setup::signatureKeyParam and Setup::encryptionKeyParam in case an ECC scheme is used
	enum EllipticCurve {
		secp112r1,
		secp128r1,
		secp160r1,
		secp192r1,
		secp224r1,
		secp256r1,
		secp384r1,
		secp521r1,

		brainpoolP160r1,
		brainpoolP192r1,
		brainpoolP224r1,
		brainpoolP256r1,
		brainpoolP320r1,
		brainpoolP384r1,
		brainpoolP512r1,

		secp112r2,
		secp128r2,
		secp160r2,

		secp160k1,
		secp192k1,
		secp224k1,
		secp256k1
	};
	Q_ENUM(EllipticCurve)

	//! Sets the maximum timeout for shutting down setups
	static void setCleanupTimeout(unsigned long timeout);
	//! Stops the datasync instance and removes it
	static void removeSetup(const QString &name, bool waitForFinished = false);

	//! Returns a list of all keystore providers defined via the plugins
	static QStringList keystoreProviders();
	//! Returns a list of all keystore providers that are actually available for use
	static QStringList availableKeystores();
	//! Checks if the given keystore provider is available
	static bool keystoreAvailable(const QString &provider);
	//! Returns the default provider to be used based on the current platform and the available providers
	static QString defaultKeystoreProvider();

	Setup();
	~Setup();

	//! @readAcFn{Setup::localDir}
	QString localDir() const;
	//! @readAcFn{Setup::remoteObjectHost}
	QUrl remoteObjectHost() const;
	//! @readAcFn{Setup::serializer}
	QJsonSerializer *serializer() const;
	//! @readAcFn{Setup::conflictResolver}
	ConflictResolver* conflictResolver() const;
	//! @readAcFn{Setup::fatalErrorHandler}
	FatalErrorHandler fatalErrorHandler() const;
	//! @readAcFn{Setup::cacheSize}
	int cacheSize() const;
	//! @readAcFn{Setup::persistDeletedVersion}
	bool persistDeletedVersion() const;
	//! @readAcFn{Setup::syncPolicy}
	SyncPolicy syncPolicy() const;
	//! @readAcFn{Setup::sslConfiguration}
	QSslConfiguration sslConfiguration() const;
	//! @readAcFn{Setup::remoteConfiguration}
	RemoteConfig remoteConfiguration() const;
	//! @readAcFn{Setup::keyStoreProvider}
	QString keyStoreProvider() const;
	//! @readAcFn{Setup::signatureScheme}
	SignatureScheme signatureScheme() const;
	//! @readAcFn{Setup::signatureKeyParam}
	QVariant signatureKeyParam() const;
	//! @readAcFn{Setup::encryptionScheme}
	EncryptionScheme encryptionScheme() const;
	//! @readAcFn{Setup::encryptionKeyParam}
	QVariant encryptionKeyParam() const;
	//! @readAcFn{Setup::cipherScheme}
	CipherScheme cipherScheme() const;
	//! @readAcFn{Setup::cipherKeySize}
	qint32 cipherKeySize() const;

	//! @writeAcFn{Setup::localDir}
	Setup &setLocalDir(QString localDir);
	//! @writeAcFn{Setup::remoteObjectHost}
	Setup &setRemoteObjectHost(QUrl remoteObjectHost);
	//! @writeAcFn{Setup::serializer}
	Setup &setSerializer(QJsonSerializer *serializer);
	//! @writeAcFn{Setup::conflictResolver}
	Setup &setConflictResolver(ConflictResolver* conflictResolver);
	//! @writeAcFn{Setup::fatalErrorHandler}
	Setup &setFatalErrorHandler(const FatalErrorHandler &fatalErrorHandler);
	//! @writeAcFn{Setup::cacheSize}
	Setup &setCacheSize(int cacheSize);
	//! @writeAcFn{Setup::persistDeletedVersion}
	Setup &setPersistDeletedVersion(bool persistDeletedVersion);
	//! @writeAcFn{Setup::syncPolicy}
	Setup &setSyncPolicy(SyncPolicy syncPolicy);
	//! @writeAcFn{Setup::sslConfiguration}
	Setup &setSslConfiguration(QSslConfiguration sslConfiguration);
	//! @writeAcFn{Setup::remoteConfiguration}
	Setup &setRemoteConfiguration(RemoteConfig remoteConfiguration);
	//! @writeAcFn{Setup::keyStoreProvider}
	Setup &setKeyStoreProvider(QString keyStoreProvider);
	//! @writeAcFn{Setup::signatureScheme}
	Setup &setSignatureScheme(SignatureScheme signatureScheme);
	//! @writeAcFn{Setup::signatureKeyParam}
	Setup &setSignatureKeyParam(QVariant signatureKeyParam);
	//! @writeAcFn{Setup::encryptionScheme}
	Setup &setEncryptionScheme(EncryptionScheme encryptionScheme);
	//! @writeAcFn{Setup::encryptionKeyParam}
	Setup &setEncryptionKeyParam(QVariant encryptionKeyParam);
	//! @writeAcFn{Setup::cipherScheme}
	Setup &setCipherScheme(CipherScheme cipherScheme);
	//! @writeAcFn{Setup::cipherKeySize}
	Setup &setCipherKeySize(qint32 cipherKeySize);

	//! @resetAcFn{Setup::localDir}
	Setup &resetLocalDir();
	//! @resetAcFn{Setup::remoteObjectHost}
	Setup &resetRemoteObjectHost();
	//! @resetAcFn{Setup::serializer}
	Setup &resetSerializer();
	//! @resetAcFn{Setup::conflictResolver}
	Setup &resetConflictResolver();
	//! @resetAcFn{Setup::fatalErrorHandler}
	Setup &resetFatalErrorHandler();
	//! @resetAcFn{Setup::cacheSize}
	Setup &resetCacheSize();
	//! @resetAcFn{Setup::persistDeletedVersion}
	Setup &resetPersistDeletedVersion();
	//! @resetAcFn{Setup::syncPolicy}
	Setup &resetSyncPolicy();
	//! @resetAcFn{Setup::sslConfiguration}
	Setup &resetSslConfiguration();
	//! @resetAcFn{Setup::remoteConfiguration}
	Setup &resetRemoteConfiguration();
	//! @resetAcFn{Setup::keyStoreProvider}
	Setup &resetKeyStoreProvider();
	//! @resetAcFn{Setup::signatureScheme}
	Setup &resetSignatureScheme();
	//! @resetAcFn{Setup::signatureKeyParam}
	Setup &resetSignatureKeyParam();
	//! @resetAcFn{Setup::encryptionScheme}
	Setup &resetEncryptionScheme();
	//! @resetAcFn{Setup::encryptionKeyParam}
	Setup &resetEncryptionKeyParam();
	//! @resetAcFn{Setup::cipherScheme}
	Setup &resetCipherScheme();
	//! @resetAcFn{Setup::cipherKeySize}
	Setup &resetCipherKeySize();

	//! Creates a datasync instance from this setup with the given name
	void create(const QString &name = DefaultSetup);
	//! Creates a passive setup with the given name that connects to the primary datasync instance
	bool createPassive(const QString &name = DefaultSetup, int timeout = 30000);

private:
	QScopedPointer<SetupPrivate> d;
};

//! Exception throw if Setup::create fails
class Q_DATASYNC_EXPORT SetupException : public Exception
{
protected:
	//! @private
	SetupException(const QString &setupName, const QString &message);
	//! @private
	SetupException(const SetupException * const other);
};

//! Exception thrown if a setup with the same name already exsits
class Q_DATASYNC_EXPORT SetupExistsException : public SetupException
{
public:
	//! Constructor with setup name
	SetupExistsException(const QString &setupName);

	QByteArray className() const noexcept override;
	void raise() const final;
	QException *clone() const final;

protected:
	//! @private
	SetupExistsException(const SetupExistsException * const other);
};

//! Exception thrown if a setups storage directory is locked by another instance
class Q_DATASYNC_EXPORT SetupLockedException : public SetupException
{
public:
	//! Constructor with setup name
	SetupLockedException(QLockFile *lockfile, const QString &setupName);

	//! The process if of the process that is locking the setup
	qint64 pid() const;
	//! The hostname of the machine that is running the process that is locking the setup
	QString hostname() const;
	//! The name of the application that is locking the setup
	QString appname() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const final;
	QException *clone() const final;

protected:
	//! @private
	SetupLockedException(const SetupLockedException *cloneFrom);

	//! @private
	qint64 _pid;
	//! @private
	QString _hostname;
	//! @private
	QString _appname;
};

// ------------- Generic Implementation -------------

template<typename TRatio>
Q_DECL_CONSTEXPR inline int ratioBytes(const intmax_t &value)
{
	return static_cast<int>(qMin(TRatio().num * value, static_cast<intmax_t>(INT_MAX)));
}

Q_DECL_CONSTEXPR inline int KB(const intmax_t &value)
{
	return ratioBytes<std::kilo>(value);
}

Q_DECL_CONSTEXPR inline int MB(const intmax_t &value)
{
	return ratioBytes<std::mega>(value);
}

Q_DECL_CONSTEXPR inline int GB(const intmax_t &value)
{
	return ratioBytes<std::giga>(value);
}

}
#endif // QTDATASYNC_SETUP_H
