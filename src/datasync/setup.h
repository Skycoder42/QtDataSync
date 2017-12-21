#ifndef QTDATASYNC_SETUP_H
#define QTDATASYNC_SETUP_H

#include <QtCore/qobject.h>
#include <QtCore/qlogging.h>
#include <QtCore/qurl.h>
#include <QtCore/qhash.h>
class QLockFile;

#include <QtNetwork/qsslconfiguration.h>

#include <functional>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"

class QJsonSerializer;

#define KB(x) (x*1024)
#define MB(x) (KB(x)*1024)
#define GB(x) (MB(x)*1024)

namespace QtDataSync {

struct Q_DATASYNC_EXPORT RemoteConfig {
	QUrl url;
	QString accessKey;
	QHash<QByteArray, QByteArray> headers;
	int keepaliveTimeout;

	RemoteConfig(const QUrl &url = {},
				 const QString &accessKey = {},
				 const QHash<QByteArray, QByteArray> &headers = {},
				 int keepaliveTimeout = 1); //1 minute between ping messages (nginx timeout is 75 seconds be default)
};

class SetupPrivate;
//! The class to setup and create datasync instances
class Q_DATASYNC_EXPORT Setup
{
	Q_GADGET
	Q_DISABLE_COPY(Setup)

	Q_PROPERTY(QString localDir READ localDir WRITE setLocalDir RESET resetLocalDir)
	Q_PROPERTY(QJsonSerializer* serializer READ serializer WRITE setSerializer)
	Q_PROPERTY(FatalErrorHandler fatalErrorHandler READ fatalErrorHandler WRITE setFatalErrorHandler RESET resetFatalErrorHandler)
	Q_PROPERTY(int cacheSize READ cacheSize WRITE setCacheSize RESET resetCacheSize)
	Q_PROPERTY(bool persistDeletedVersion READ persistDeletedVersion WRITE setPersistDeletedVersion)
	Q_PROPERTY(QSslConfiguration sslConfiguration READ sslConfiguration WRITE setSslConfiguration RESET resetSslConfiguration)
	Q_PROPERTY(RemoteConfig remoteConfiguration READ remoteConfiguration WRITE setRemoteConfiguration)
	Q_PROPERTY(QString keyStoreProvider READ keyStoreProvider WRITE setKeyStoreProvider RESET resetKeyStoreProvider)
	Q_PROPERTY(SignatureScheme signatureScheme READ signatureScheme WRITE setSignatureScheme RESET resetSignatureScheme)
	Q_PROPERTY(QVariant signatureKeyParam READ signatureKeyParam WRITE setSignatureKeyParam RESET resetSignatureKeyParam)
	Q_PROPERTY(EncryptionScheme encryptionScheme READ encryptionScheme WRITE setEncryptionScheme RESET resetEncryptionScheme)
	Q_PROPERTY(QVariant encryptionKeyParam READ encryptionKeyParam WRITE setEncryptionKeyParam RESET resetEncryptionKeyParam)
	Q_PROPERTY(CipherScheme cipherScheme READ cipherScheme WRITE setCipherScheme RESET resetCipherScheme)
	Q_PROPERTY(qint32 cipherKeySize READ cipherKeySize WRITE setCipherKeySize RESET resetCipherKeySize)

public:
	typedef std::function<void (QString, QString, const QMessageLogContext &)> FatalErrorHandler;

	enum SignatureScheme {
		RSA_PSS_SHA3_512,
		ECDSA_ECP_SHA3_512,
		ECNR_ECP_SHA3_512
	};
	Q_ENUM(SignatureScheme)

	enum EncryptionScheme {
		RSA_OAEP_SHA3_512
	};
	Q_ENUM(EncryptionScheme)

	enum CipherScheme
	{
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

		//NOTE CRYPTOPP6 add curve25519
	};
	Q_ENUM(EllipticCurve)

	//! Sets the maximum timeout for shutting down setups
	static void setCleanupTimeout(unsigned long timeout);
	//! Stops the datasync instance and removes it
	static void removeSetup(const QString &name, bool waitForFinished = false);

	//! Constructor
	Setup();
	//! Destructor
	~Setup();

	//! Returns the setups local directory
	QString localDir() const;
	//! Returns the setups json serializer
	QJsonSerializer *serializer() const;
	//! Returns the fatal error handler to be used by the Logger
	FatalErrorHandler fatalErrorHandler() const;
	int cacheSize() const;
	bool persistDeletedVersion() const;
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
	//! Sets the setups json serializer
	Setup &setSerializer(QJsonSerializer *serializer);
	//! Sets the fatal error handler to be used by the Logger
	Setup &setFatalErrorHandler(const FatalErrorHandler &fatalErrorHandler);
	Setup &setCacheSize(int cacheSize);
	Setup &setPersistDeletedVersion(bool persistDeletedVersion);
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
	Setup &resetFatalErrorHandler();
	Setup &resetCacheSize();
	Setup &resetSslConfiguration();
	Setup &resetKeyStoreProvider();
	Setup &resetSignatureScheme();
	Setup &resetSignatureKeyParam();
	Setup &resetEncryptionScheme();
	Setup &resetEncryptionKeyParam();
	Setup &resetCipherScheme();
	Setup &resetCipherKeySize();

	//! Creates a datasync instance from this setup with the given name
	void create(const QString &name = DefaultSetup);

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

}

Q_DECLARE_METATYPE(QtDataSync::RemoteConfig)

#endif // QTDATASYNC_SETUP_H
