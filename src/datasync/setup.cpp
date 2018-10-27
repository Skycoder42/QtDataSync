#include "defaults.h"
#include "setup.h"
#include "setup_p.h"
#include "defaults_p.h"
#include "keystore.h"
#include "message_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QLockFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QThreadStorage>
#include <QtCore/QLoggingCategory>
#include <QtCore/QEventLoop>

#include "threadedserver_p.h"

Q_LOGGING_CATEGORY(qdssetup, "qtdatasync.setup", QtInfoMsg)

using namespace QtDataSync;

void Setup::setCleanupTimeout(unsigned long timeout)
{
	SetupPrivate::timeout = timeout;
}

void Setup::removeSetup(const QString &name, bool waitForFinished)
{
	QMutexLocker _(&SetupPrivate::setupMutex);
	if(!SetupPrivate::engines.contains(name))
		return;
	auto mThread = SetupPrivate::engines.value(name);
	_.unlock(); //unlock first

	if(mThread) {
		mThread->stopEngine();
		if(waitForFinished)
			mThread->waitAndTerminate(SetupPrivate::timeout);
	} else {
		//was set but null -> must be passive -> remove passive defaults
		DefaultsPrivate::removeDefaults(name);
		// and remove from setup
		_.relock();
		SetupPrivate::engines.remove(name);
	}
}

QStringList Setup::keystoreProviders()
{
	return CryptoController::allKeystoreKeys();
}

QStringList Setup::availableKeystores()
{
	return CryptoController::availableKeystoreKeys();
}

bool Setup::keystoreAvailable(const QString &provider)
{
	return CryptoController::keystoreAvailable(provider);
}

#define RETURN_IF_AVAILABLE(x) \
	if(CryptoController::keystoreAvailable(x)) \
		return x

QString Setup::defaultKeystoreProvider()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	auto prefered = qEnvironmentVariable("QTDATASYNC_KEYSTORE");
#else
	auto prefered = QString::fromUtf8(qgetenv("QTDATASYNC_KEYSTORE"));
#endif
	if(!prefered.isEmpty())
		RETURN_IF_AVAILABLE(prefered);
#ifdef Q_OS_WIN
	RETURN_IF_AVAILABLE(QStringLiteral("wincred"));
#endif
#ifdef Q_OS_DARWIN
	RETURN_IF_AVAILABLE(QStringLiteral("keychain"));
#endif
	//mostly used on linux, but theoretically possible on any platform
	RETURN_IF_AVAILABLE(QStringLiteral("secretservice"));
	RETURN_IF_AVAILABLE(QStringLiteral("kwallet"));
	return QStringLiteral("plain");
}

KeyStore *Setup::loadKeystore(QObject *parent, const QString &setupName)
{
	return loadKeystore(defaultKeystoreProvider(), parent, setupName);
}

KeyStore *Setup::loadKeystore(const QString &provider, QObject *parent, const QString &setupName)
{
	return CryptoController::loadKeystore(provider, parent, setupName);
}

#undef RETURN_IF_AVAILABLE

Setup::Setup() :
	d{new SetupPrivate()}
{}

Setup::~Setup() = default;

QString Setup::localDir() const
{
	return d->localDir;
}

QUrl Setup::remoteObjectHost() const
{
	return d->roAddress;
}

QJsonSerializer *Setup::serializer() const
{
	return d->serializer.data();
}

ConflictResolver *Setup::conflictResolver() const
{
	return d->resolver.data();
}

Setup::FatalErrorHandler Setup::fatalErrorHandler() const
{
	return d->fatalErrorHandler;
}

int Setup::cacheSize() const
{
	return d->properties.value(Defaults::CacheSize).toInt();
}

bool Setup::persistDeletedVersion() const
{
	return d->properties.value(Defaults::PersistDeleted).toBool();
}

Setup::SyncPolicy Setup::syncPolicy() const
{
	return static_cast<SyncPolicy>(d->properties.value(Defaults::ConflictPolicy).toInt());
}

QSslConfiguration Setup::sslConfiguration() const
{
	return d->properties.value(Defaults::SslConfiguration).value<QSslConfiguration>();
}

RemoteConfig Setup::remoteConfiguration() const
{
	return d->properties.value(Defaults::RemoteConfiguration).value<RemoteConfig>();
}

QString Setup::keyStoreProvider() const
{
	return d->properties.value(Defaults::KeyStoreProvider).toString();
}

Setup::SignatureScheme Setup::signatureScheme() const
{
	return static_cast<SignatureScheme>(d->properties.value(Defaults::SignScheme).toInt());
}

QVariant Setup::signatureKeyParam() const
{
	return d->properties.value(Defaults::SignKeyParam);
}

Setup::EncryptionScheme Setup::encryptionScheme() const
{
	return static_cast<EncryptionScheme>(d->properties.value(Defaults::CryptScheme).toInt());
}

QVariant Setup::encryptionKeyParam() const
{
	return d->properties.value(Defaults::CryptKeyParam);
}

Setup::CipherScheme Setup::cipherScheme() const
{
	return static_cast<CipherScheme>(d->properties.value(Defaults::SymScheme).toInt());
}

qint32 Setup::cipherKeySize() const
{
	return d->properties.value(Defaults::SymKeyParam).toInt();
}

Setup::EventMode Setup::eventLoggingMode() const
{
	return d->properties.value(Defaults::EventLoggingMode).value<EventMode>();
}

Setup &Setup::setLocalDir(QString localDir)
{
	d->localDir = std::move(localDir);
	return *this;
}

Setup &Setup::setRemoteObjectHost(QUrl remoteObjectHost)
{
	d->roAddress = std::move(remoteObjectHost);
	return *this;
}

Setup &Setup::setSerializer(QJsonSerializer *serializer)
{
	Q_ASSERT_X(serializer, Q_FUNC_INFO, "Serializer must not be null");
	d->serializer.reset(serializer);
	return *this;
}

Setup &Setup::setConflictResolver(ConflictResolver *conflictResolver)
{
	d->resolver.reset(conflictResolver);
	return *this;
}

Setup &Setup::setFatalErrorHandler(const FatalErrorHandler &fatalErrorHandler)
{
	d->fatalErrorHandler = fatalErrorHandler;//MAJOR move
	return *this;
}

Setup &Setup::setCacheSize(int cacheSize)
{
	d->properties.insert(Defaults::CacheSize, cacheSize);
	return *this;
}

Setup &Setup::setPersistDeletedVersion(bool persistDeletedVersion)
{
	d->properties.insert(Defaults::PersistDeleted, persistDeletedVersion);
	return *this;
}

Setup &Setup::setSyncPolicy(Setup::SyncPolicy syncPolicy)
{
	d->properties.insert(Defaults::ConflictPolicy, syncPolicy);
	return *this;
}

Setup &Setup::setSslConfiguration(QSslConfiguration sslConfiguration)
{
	d->properties.insert(Defaults::SslConfiguration, QVariant::fromValue(std::move(sslConfiguration)));
	return *this;
}

Setup &Setup::setRemoteConfiguration(RemoteConfig remoteConfiguration)
{
	d->properties.insert(Defaults::RemoteConfiguration, QVariant::fromValue(std::move(remoteConfiguration)));
	return *this;
}

Setup &Setup::setKeyStoreProvider(QString keyStoreProvider)
{
	d->properties.insert(Defaults::KeyStoreProvider, std::move(keyStoreProvider));
	return *this;
}

Setup &Setup::setSignatureScheme(Setup::SignatureScheme signatureScheme)
{
	d->properties.insert(Defaults::SignScheme, signatureScheme);
	return *this;
}

Setup &Setup::setSignatureKeyParam(QVariant signatureKeyParam)
{
	d->properties.insert(Defaults::SignKeyParam, std::move(signatureKeyParam));
	return *this;
}

Setup &Setup::setEncryptionScheme(Setup::EncryptionScheme encryptionScheme)
{
	d->properties.insert(Defaults::CryptScheme, encryptionScheme);
	return *this;
}

Setup &Setup::setEncryptionKeyParam(QVariant encryptionKeyParam)
{
	d->properties.insert(Defaults::CryptKeyParam, std::move(encryptionKeyParam));
	return *this;
}

Setup &Setup::setCipherScheme(Setup::CipherScheme cipherScheme)
{
	d->properties.insert(Defaults::SymScheme, cipherScheme);
	return *this;
}

Setup &Setup::setCipherKeySize(qint32 cipherKeySize)
{
	d->properties.insert(Defaults::SymKeyParam, cipherKeySize);
	return *this;
}

Setup &Setup::setEventLoggingMode(Setup::EventMode eventLoggingMode)
{
	d->properties.insert(Defaults::EventLoggingMode, QVariant::fromValue(eventLoggingMode));
	return *this;
}

Setup &Setup::resetLocalDir()
{
	d->localDir = SetupPrivate::DefaultLocalDir;
	return *this;
}

Setup &Setup::resetRemoteObjectHost()
{
	d->roAddress.clear();
	return *this;
}

Setup &Setup::resetSerializer()
{
	return setSerializer(new QJsonSerializer());
}

Setup &Setup::resetConflictResolver()
{
	d->resolver.reset();
	return *this;
}

Setup &Setup::resetFatalErrorHandler()
{
	d->fatalErrorHandler = FatalErrorHandler();
	return *this;
}

Setup &Setup::resetCacheSize()
{
	d->properties.insert(Defaults::CacheSize, MB(100));
	return *this;
}

Setup &Setup::resetPersistDeletedVersion()
{
	d->properties.insert(Defaults::PersistDeleted, false);
	return *this;
}

Setup &Setup::resetSyncPolicy()
{
	d->properties.insert(Defaults::ConflictPolicy, PreferChanged);
	return *this;
}

Setup &Setup::resetSslConfiguration()
{
	return setSslConfiguration(QSslConfiguration::defaultConfiguration());
}

Setup &Setup::resetRemoteConfiguration()
{
	d->properties.remove(Defaults::RemoteConfiguration);
	return *this;
}

Setup &Setup::resetKeyStoreProvider()
{
	d->properties.remove(Defaults::KeyStoreProvider);
	return *this;
}

Setup &Setup::resetSignatureScheme()
{
	d->properties.insert(Defaults::SignScheme, ECDSA_ECP_SHA3_512);
	return *this;
}

Setup &Setup::resetSignatureKeyParam()
{
	d->properties.remove(Defaults::SignKeyParam);
	return *this;
}

Setup &Setup::resetEncryptionScheme()
{
	d->properties.insert(Defaults::CryptScheme, ECIES_ECP_SHA3_512);
	return *this;
}

Setup &Setup::resetEncryptionKeyParam()
{
	d->properties.remove(Defaults::CryptKeyParam);
	return *this;
}

Setup &Setup::resetCipherScheme()
{
	d->properties.insert(Defaults::SymScheme, AES_EAX);
	return *this;
}

Setup &Setup::resetCipherKeySize()
{
	d->properties.remove(Defaults::SymKeyParam);
	return *this;
}

Setup &Setup::resetEventLoggingMode()
{
	return setEventLoggingMode(EventMode::Unchanged);
}

Setup &Setup::setAccount(const QJsonObject &importData, bool keepData, bool allowFailure)
{
	d->initialImport = ExchangeEngine::ImportData {
		importData,
		{},
		keepData,
		allowFailure
	};
	return *this;
}

Setup &Setup::setAccount(const QByteArray &importData, bool keepData, bool allowFailure)
{
	try {
		return setAccount(SetupPrivate::parseObj(importData), keepData, allowFailure);
	} catch(QString &s) {
		throw Exception(QStringLiteral("<Unnamed>"), s);
	}
}

Setup &Setup::setAccountTrusted(const QJsonObject &importData, const QString &password, bool keepData, bool allowFailure)
{
	if(password.isEmpty())
		throw Exception(QStringLiteral("<Unnamed>"), ExchangeEngine::tr("Password for trusted import must not be empty"));

	d->initialImport = ExchangeEngine::ImportData {
		importData,
		password,
		keepData,
		allowFailure
	};
	return *this;
}

Setup &Setup::setAccountTrusted(const QByteArray &importData, const QString &password, bool keepData, bool allowFailure)
{
	try {
		return setAccountTrusted(SetupPrivate::parseObj(importData), password, keepData, allowFailure);
	} catch(QString &s) {
		throw Exception(QStringLiteral("<Unnamed>"), s);
	}
}

void Setup::create(const QString &name)
{
	QMutexLocker _(&SetupPrivate::setupMutex);
	if(SetupPrivate::engines.contains(name))
		throw SetupExistsException(name);

	// create storage dir
	auto storageDir = d->createStorageDir(name);

	// lock the setup
	auto lockFile = new QLockFile(storageDir.absoluteFilePath(QStringLiteral(".lock")));
	if(!lockFile->tryLock())
		throw SetupLockedException(lockFile, name);
	qCDebug(qdssetup) << "Created lockfile for setup" << name;

	// create defaults + engine
	d->createDefaults(name, storageDir, false);
	auto engine = new ExchangeEngine(name, d->fatalErrorHandler);
	if(d->initialImport.isSet())
		engine->prepareInitialAccount(d->initialImport);

	// create and start the engine in its thread
	QSharedPointer<EngineThread> thread {
		new EngineThread{name, engine, lockFile},
		&SetupPrivate::deleteThread
	};
	SetupPrivate::engines.insert(name, thread);
	thread->startEngine();
}

bool Setup::createPassive(const QString &name, int timeout)
{
	QMutexLocker _(&SetupPrivate::setupMutex);
	if(SetupPrivate::engines.contains(name))
		throw SetupExistsException(name);

	// create storage dir and defaults
	auto storageDir = d->createStorageDir(name);
	d->createDefaults(name, storageDir, true);

	// theoretically "ready" -> save (with null engine) and unlock
	SetupPrivate::engines.insert(name, {});
	_.unlock();

	//wait for the setup to complete initialization
	auto defaults = DefaultsPrivate::obtainDefaults(name);
	auto isCreated = false;
	QEventLoop waitLoop;
	QObject::connect(defaults.data(), &DefaultsPrivate::passiveCreated,
					 &waitLoop, [&]() {
		isCreated = true;
		if(timeout == 0)
			waitLoop.quit();
	});
	QObject::connect(defaults.data(), &DefaultsPrivate::passiveReady,
					 &waitLoop, &QEventLoop::quit);
	if(timeout > 0) {
		QTimer::singleShot(timeout, &waitLoop, [&]() {
			if(isCreated)
				waitLoop.exit(EXIT_FAILURE);
			else //not created yet -> force exit once it is
				timeout = 0;
		});
	}

	auto res = waitLoop.exec();
	Q_ASSERT_X(isCreated, Q_FUNC_INFO, "Timeout to low for the eventloop to pickup the object creation");
	return res == EXIT_SUCCESS;
}

// ------------- Private Implementation -------------

const QString SetupPrivate::DefaultLocalDir = QStringLiteral("./qtdatasync/default");
QMutex SetupPrivate::setupMutex(QMutex::Recursive);
QHash<QString, QSharedPointer<EngineThread>> SetupPrivate::engines;
unsigned long SetupPrivate::timeout = ULONG_MAX;

void SetupPrivate::cleanupHandler()
{
	QMutexLocker _(&setupMutex);
	auto threads = engines;
	engines.clear();
	_.unlock();

	// stop/remove all running setups
	for(auto it = threads.constBegin(); it != threads.constEnd(); it++) {
		const auto &thread = it.value();
		if(thread) //active setup -> send stip
			thread->stopEngine();
		else //passive setup -> simply remove defaults
			DefaultsPrivate::removeDefaults(it.key());
	}

	// wait for active setups to finish
	for(auto &thread : threads) {
		if(thread)
			thread->waitAndTerminate(timeout);
	}
}

unsigned long SetupPrivate::currentTimeout()
{
	return timeout;
}

ExchangeEngine *SetupPrivate::engine(const QString &setupName)
{
	QMutexLocker _(&setupMutex);
	auto engine = engines.value(setupName)->engine();
	if(!engine)
		throw SetupDoesNotExistException(setupName);
	else
		return engine;
}

QDir SetupPrivate::createStorageDir(const QString &setupName)
{
	QDir storageDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	if(!storageDir.cd(localDir)) {
		storageDir.mkpath(localDir);
		storageDir.cd(localDir);
		QFile::setPermissions(storageDir.absolutePath(),
							  QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser);
		qCDebug(qdssetup) << "Created storage directory for setup" << setupName;
	}
	return storageDir;
}

void SetupPrivate::createDefaults(const QString &setupName, const QDir &storageDir, bool passive)
{
	//determine the ro address
	if(!roAddress.isValid()) {
		roAddress.clear();
		roAddress.setScheme(ThreadedServer::UrlScheme());
		roAddress.setPath(QStringLiteral("/qtdatasync/%2/enginenode").arg(setupName));
		qCDebug(qdssetup) << "Using default remote object address for setup" << setupName << "as" << roAddress;
	}

	// create defaults + engine
	DefaultsPrivate::createDefaults(setupName,
									passive,
									storageDir,
									roAddress,
									properties,
									serializer.take(),
									resolver.take());
}

QJsonObject SetupPrivate::parseObj(const QByteArray &data)
{
	QJsonParseError error;
	const auto doc = QJsonDocument::fromJson(data, &error);
	if(error.error != QJsonParseError::NoError)
		throw error.errorString();
	if(!doc.isObject())
		throw ExchangeEngine::tr("Import data must be a JSON object");
	return doc.object();
}

SetupPrivate::SetupPrivate() :
	localDir{DefaultLocalDir},
	serializer{new QJsonSerializer()},
	properties{
		{Defaults::CacheSize, MB(100)},
		{Defaults::PersistDeleted, false},
		{Defaults::ConflictPolicy, Setup::PreferChanged},
		{Defaults::SslConfiguration, QVariant::fromValue(QSslConfiguration::defaultConfiguration())},
		{Defaults::SignScheme, Setup::ECDSA_ECP_SHA3_512},
		{Defaults::CryptScheme, Setup::ECIES_ECP_SHA3_512},
		{Defaults::SymScheme, Setup::AES_EAX},
		{Defaults::EventLoggingMode, QVariant::fromValue(Setup::EventMode::Unchanged)}
	}
{}

void SetupPrivate::deleteThread(EngineThread *thread)
{
	delete thread;
}

// ------------- Exceptions -------------

SetupException::SetupException(const QString &setupName, const QString &message) :
	Exception{setupName, message}
{}

SetupException::SetupException(const SetupException * const other) :
	Exception{other}
{}



SetupExistsException::SetupExistsException(const QString &setupName) :
	SetupException{setupName, QStringLiteral("Failed to create setup! A setup with the given name already exists!")}
{}

SetupExistsException::SetupExistsException(const SetupExistsException *const other) :
	SetupException{other}
{}

QByteArray SetupExistsException::className() const noexcept
{
	return QTDATASYNC_EXCEPTION_NAME(SetupExistsException);
}

void SetupExistsException::raise() const
{
	throw (*this);
}

QException *SetupExistsException::clone() const
{
	return new SetupExistsException(this);
}



SetupLockedException::SetupLockedException(QLockFile *lockfile, const QString &setupName) :
	SetupException{setupName, QString()}
{
	switch(lockfile->error()) {
	case QLockFile::LockFailedError:
		_message = QStringLiteral("Failed to lock storage directory, already locked by another process!");
		if(!lockfile->getLockInfo(&_pid, &_hostname, &_appname)) {
			_pid = -1;
			_hostname = QStringLiteral("<unknown>");
			_appname = _hostname;
		}
		break;
	case QLockFile::PermissionError:
		_message = QStringLiteral("Failed to lock storage directory, no permission to create lockfile!");
		break;
	case QLockFile::NoError:
	case QLockFile::UnknownError:
		_message = QStringLiteral("Failed to lock storage directory with unknown error!");
		break;

	}

	delete lockfile;
}

SetupLockedException::SetupLockedException(const SetupLockedException *const other) :
	SetupException{other}
{}

QByteArray SetupLockedException::className() const noexcept
{
	return QTDATASYNC_EXCEPTION_NAME(SetupLockedException);
}

QString SetupLockedException::qWhat() const
{
	QString msg = Exception::qWhat();
	if(_pid != 0) {
		msg += QStringLiteral("\n\tLocked by:"
							  "\n\t\tProcess-ID: %1"
							  "\n\t\tHostname: %2"
							  "\n\t\tAppname: %3")
			   .arg(_pid)
			   .arg(_hostname, _appname);
	}
	return msg;
}

void SetupLockedException::raise() const
{
	throw (*this);
}

QException *SetupLockedException::clone() const
{
	return new SetupLockedException(this);
}

// ------------- Application hooks -------------

namespace {

void initCleanupHandlers()
{
	qRegisterMetaType<QThread*>();
	qAddPostRoutine(SetupPrivate::cleanupHandler);
}

}

Q_COREAPP_STARTUP_FUNCTION(initCleanupHandlers)
