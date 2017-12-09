#ifndef QTDATASYNC_SETUP_H
#define QTDATASYNC_SETUP_H

#include <QtCore/qobject.h>
#include <QtCore/qlogging.h>
class QLockFile;

#include <QtNetwork/qsslconfiguration.h>

#include <functional>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/defaults.h"
#include "QtDataSync/exception.h"

class QJsonSerializer;

#define KB(x) (x*1024)
#define MB(x) (KB(x)*1024)
#define GB(x) (MB(x)*1024)

namespace QtDataSync {

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
	Q_PROPERTY(QSslConfiguration sslConfiguration READ sslConfiguration WRITE setSslConfiguration RESET resetSslConfiguration)
	Q_PROPERTY(RemoteConfig remoteConfiguration READ remoteConfiguration WRITE setRemoteConfiguration) //TODO allow changing via sync/exchange manager?
	Q_PROPERTY(QString keyStoreProvider READ keyStoreProvider WRITE setKeyStoreProvider RESET resetKeyStoreProvider)
	Q_PROPERTY(quint32 rsaKeySize READ rsaKeySize WRITE setRsaKeySize RESET resetRsaKeySize)

public:
	typedef std::function<void (QString, QString, const QMessageLogContext &)> FatalErrorHandler;

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
	QSslConfiguration sslConfiguration() const;
	RemoteConfig remoteConfiguration() const;
	QString keyStoreProvider() const;
	quint32 rsaKeySize() const;

	//! Sets the setups local directory
	Setup &setLocalDir(QString localDir);
	//! Sets the setups json serializer
	Setup &setSerializer(QJsonSerializer *serializer);
	//! Sets the fatal error handler to be used by the Logger
	Setup &setFatalErrorHandler(const FatalErrorHandler &fatalErrorHandler);
	Setup &setCacheSize(int cacheSize);
	Setup &setSslConfiguration(QSslConfiguration sslConfiguration);
	Setup &setRemoteConfiguration(RemoteConfig remoteConfiguration);
	Setup &setKeyStoreProvider(QString keyStoreProvider);
	Setup &setRsaKeySize(quint32 rsaKeySize);

	Setup &resetLocalDir();
	Setup &resetFatalErrorHandler();
	Setup &resetCacheSize();
	Setup &resetSslConfiguration();
	Setup &resetKeyStoreProvider();
	Setup &resetRsaKeySize();

	//! Creates a datasync instance from this setup with the given name
	void create(const QString &name = DefaultSetup);

private:
	QScopedPointer<SetupPrivate> d;
	quint32 m_rsaKeySize;
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

#endif // QTDATASYNC_SETUP_H
