#ifndef QTDATASYNC_SETUP_H
#define QTDATASYNC_SETUP_H

#include <QtCore/qobject.h>
#include <QtCore/qlogging.h>
#include <QtCore/qurl.h>
#include <QtCore/qhash.h>
#include <QtCore/qshareddata.h>
class QLockFile;

#include <QtNetwork/qsslconfiguration.h>

#include <functional>
#include <ratio>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"

class QJsonSerializer;

namespace QtDataSync {

template<typename TRatio>
Q_DECL_CONSTEXPR inline int ratioBytes(const intmax_t &value);
Q_DECL_CONSTEXPR inline int KB(const intmax_t &value);
Q_DECL_CONSTEXPR inline int MB(const intmax_t &value);
Q_DECL_CONSTEXPR inline int GB(const intmax_t &value);

class ConflictResolver;

class RemoteConfigPrivate;
class Q_DATASYNC_EXPORT RemoteConfig
{
	Q_GADGET

	Q_PROPERTY(QUrl url READ url WRITE setUrl)
	Q_PROPERTY(QString accessKey READ accessKey WRITE setAccessKey)
	Q_PROPERTY(HeaderHash headers READ headers WRITE setHeaders)
	Q_PROPERTY(int keepaliveTimeout READ keepaliveTimeout WRITE setKeepaliveTimeout)

public:
	typedef QHash<QByteArray, QByteArray> HeaderHash;

	RemoteConfig(const QUrl &url = {},
				 const QString &accessKey = {},
				 const HeaderHash &headers = {},
				 int keepaliveTimeout = 1); //1 minute between ping messages (nginx timeout is 75 seconds be default)
	RemoteConfig(const RemoteConfig &other);
	~RemoteConfig();

	RemoteConfig &operator=(const RemoteConfig &other);

	QUrl url() const;
	QString accessKey() const;
	HeaderHash headers() const;
	int keepaliveTimeout() const;

	void setUrl(QUrl url);
	void setAccessKey(QString accessKey);
	void setHeaders(HeaderHash headers);
	void setKeepaliveTimeout(int keepaliveTimeout);

private:
	QSharedDataPointer<RemoteConfigPrivate> d;

	friend Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RemoteConfig &deviceInfo);
	friend Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RemoteConfig &deviceInfo);
};

class SetupPrivate;
//! The class to setup and create datasync instances
class Q_DATASYNC_EXPORT Setup
{
	Q_GADGET
	Q_DISABLE_COPY(Setup)

	Q_PROPERTY(QString localDir READ localDir WRITE setLocalDir RESET resetLocalDir)
	Q_PROPERTY(QUrl remoteObjectHost READ remoteObjectHost WRITE setRemoteObjectHost RESET resetRemoteObjectHost)
	Q_PROPERTY(QJsonSerializer* serializer READ serializer WRITE setSerializer RESET resetSerializer)
	Q_PROPERTY(ConflictResolver* conflictResolver READ conflictResolver WRITE setConflictResolver RESET resetConflictResolver)
	Q_PROPERTY(FatalErrorHandler fatalErrorHandler READ fatalErrorHandler WRITE setFatalErrorHandler RESET resetFatalErrorHandler)
	Q_PROPERTY(int cacheSize READ cacheSize WRITE setCacheSize RESET resetCacheSize)
	Q_PROPERTY(bool persistDeletedVersion READ persistDeletedVersion WRITE setPersistDeletedVersion RESET resetPersistDeletedVersion)
	Q_PROPERTY(SyncPolicy syncPolicy READ syncPolicy WRITE setSyncPolicy RESET resetSyncPolicy)
	Q_PROPERTY(QSslConfiguration sslConfiguration READ sslConfiguration WRITE setSslConfiguration RESET resetSslConfiguration)
	Q_PROPERTY(RemoteConfig remoteConfiguration READ remoteConfiguration WRITE setRemoteConfiguration RESET resetRemoteConfiguration)
	Q_PROPERTY(QString keyStoreProvider READ keyStoreProvider WRITE setKeyStoreProvider RESET resetKeyStoreProvider)
	Q_PROPERTY(SignatureScheme signatureScheme READ signatureScheme WRITE setSignatureScheme RESET resetSignatureScheme)
	Q_PROPERTY(QVariant signatureKeyParam READ signatureKeyParam WRITE setSignatureKeyParam RESET resetSignatureKeyParam)
	Q_PROPERTY(EncryptionScheme encryptionScheme READ encryptionScheme WRITE setEncryptionScheme RESET resetEncryptionScheme)
	Q_PROPERTY(QVariant encryptionKeyParam READ encryptionKeyParam WRITE setEncryptionKeyParam RESET resetEncryptionKeyParam)
	Q_PROPERTY(CipherScheme cipherScheme READ cipherScheme WRITE setCipherScheme RESET resetCipherScheme)
	Q_PROPERTY(qint32 cipherKeySize READ cipherKeySize WRITE setCipherKeySize RESET resetCipherKeySize)

public:
	typedef std::function<void (QString, QString, const QMessageLogContext &)> FatalErrorHandler;

	enum SyncPolicy {
		PreferChanged,
		PreferDeleted
	};
	Q_ENUM(SyncPolicy)

	enum SignatureScheme {
		RSA_PSS_SHA3_512,
		ECDSA_ECP_SHA3_512,
		ECNR_ECP_SHA3_512
	};
	Q_ENUM(SignatureScheme)

	enum EncryptionScheme {
		RSA_OAEP_SHA3_512,
		ECIES_ECP_SHA3_512
	};
	Q_ENUM(EncryptionScheme)

	enum CipherScheme {
		AES_EAX,
		AES_GCM,
		TWOFISH_EAX,
		TWOFISH_GCM,
		SERPENT_EAX,
		SERPENT_GCM,
		IDEA_EAX,
	};
	Q_ENUM(CipherScheme)

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

	static QStringList keystoreProviders();
	static QStringList availableKeystores();
	static bool keystoreAvailable(const QString &provider);
	static QString defaultKeystoreProvider();

	//! Constructor
	Setup();
	//! Destructor
	~Setup();

	//! Returns the setups local directory
	QString localDir() const;
	QUrl remoteObjectHost() const;
	//! Returns the setups json serializer
	QJsonSerializer *serializer() const;
	ConflictResolver* conflictResolver() const;
	//! Returns the fatal error handler to be used by the Logger
	FatalErrorHandler fatalErrorHandler() const;
	int cacheSize() const;
	bool persistDeletedVersion() const;
	SyncPolicy syncPolicy() const;
	QSslConfiguration sslConfiguration() const;
	RemoteConfig remoteConfiguration() const;
	QString keyStoreProvider() const;
	SignatureScheme signatureScheme() const;
	QVariant signatureKeyParam() const;
	EncryptionScheme encryptionScheme() const;
	QVariant encryptionKeyParam() const;
	CipherScheme cipherScheme() const;
	qint32 cipherKeySize() const;

	//! Sets the setups local directory
	Setup &setLocalDir(QString localDir);
	Setup &setRemoteObjectHost(QUrl remoteObjectHost);
	//! Sets the setups json serializer
	Setup &setSerializer(QJsonSerializer *serializer);
	Setup &setConflictResolver(ConflictResolver* conflictResolver);
	//! Sets the fatal error handler to be used by the Logger
	Setup &setFatalErrorHandler(const FatalErrorHandler &fatalErrorHandler);
	Setup &setCacheSize(int cacheSize);
	Setup &setPersistDeletedVersion(bool persistDeletedVersion);
	Setup &setSyncPolicy(SyncPolicy syncPolicy);
	Setup &setSslConfiguration(QSslConfiguration sslConfiguration);
	Setup &setRemoteConfiguration(RemoteConfig remoteConfiguration);
	Setup &setKeyStoreProvider(QString keyStoreProvider);
	Setup &setSignatureScheme(SignatureScheme signatureScheme);
	Setup &setSignatureKeyParam(QVariant signatureKeyParam);
	Setup &setEncryptionScheme(EncryptionScheme encryptionScheme);
	Setup &setEncryptionKeyParam(QVariant encryptionKeyParam);
	Setup &setCipherScheme(CipherScheme cipherScheme);
	Setup &setCipherKeySize(qint32 cipherKeySize);

	Setup &resetLocalDir();
	Setup &resetRemoteObjectHost();
	Setup &resetSerializer();
	Setup &resetConflictResolver();
	Setup &resetFatalErrorHandler();
	Setup &resetCacheSize();
	Setup &resetPersistDeletedVersion();
	Setup &resetSyncPolicy();
	Setup &resetSslConfiguration();
	Setup &resetRemoteConfiguration();
	Setup &resetKeyStoreProvider();
	Setup &resetSignatureScheme();
	Setup &resetSignatureKeyParam();
	Setup &resetEncryptionScheme();
	Setup &resetEncryptionKeyParam();
	Setup &resetCipherScheme();
	Setup &resetCipherKeySize();

	//! Creates a datasync instance from this setup with the given name
	void create(const QString &name = DefaultSetup);
	bool createPassive(const QString &name = DefaultSetup, int timeout = 30000);

private:
	QScopedPointer<SetupPrivate> d;
};

//! Exception throw if Setup::create fails
class Q_DATASYNC_EXPORT SetupException : public Exception
{
protected:
	//! Constructor with error message and setup name
	SetupException(const QString &setupName, const QString &message);
	//! Constructor that clones another exception
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
	//! Constructor that clones another exception
	SetupExistsException(const SetupExistsException * const other);
};

//! Exception thrown if a setups storage directory is locked by another instance
class Q_DATASYNC_EXPORT SetupLockedException : public SetupException
{
public:
	//! Constructor with setup name
	SetupLockedException(QLockFile *lockfile, const QString &setupName);

	qint64 pid() const;
	QString hostname() const;
	QString appname() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const final;
	QException *clone() const final;

protected:
	//! Constructor that clones another exception
	SetupLockedException(const SetupLockedException *cloneFrom);

	qint64 _pid;
	QString _hostname;
	QString _appname;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RemoteConfig &deviceInfo);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RemoteConfig &deviceInfo);

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

Q_DECLARE_METATYPE(QtDataSync::RemoteConfig)

#endif // QTDATASYNC_SETUP_H
