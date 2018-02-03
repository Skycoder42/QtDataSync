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
	if(SetupPrivate::engines.contains(name)) {
		auto &info = SetupPrivate::engines[name];
		if(info.engine) {
			qCDebug(qdssetup) << "Finalizing engine of setup" << name;
			QMetaObject::invokeMethod(info.engine, "finalize", Qt::QueuedConnection);
			info.engine = nullptr;
		} else
			qCDebug(qdssetup) << "Engine of setup" << name << "already finalizing";

		if(waitForFinished) {
			if(!info.thread->wait(SetupPrivate::timeout)) {
				qCWarning(qdssetup) << "Workerthread of setup" << name << "did not finish before the timout. Terminating...";
				info.thread->terminate();
				auto wRes = info.thread->wait(100);
				qCDebug(qdssetup) << "Terminate result for setup" << name << ":"
								  << wRes;
			}
			info.thread->deleteLater();
			QCoreApplication::processEvents();//required to perform queued events
		}
	} else //no there -> remove defaults (either already removed does nothing, or remove passive)
		DefaultsPrivate::removeDefaults(name);
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

#undef RETURN_IF_AVAILABLE

Setup::Setup() :
	d(new SetupPrivate())
{}

Setup::~Setup() {}

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
	return d->properties.value(Defaults::SymKeyParam).toUInt();
}

Setup &Setup::setLocalDir(QString localDir)
{
	d->localDir = localDir;
	return *this;
}

Setup &Setup::setRemoteObjectHost(QUrl remoteObjectHost)
{
	d->roAddress = remoteObjectHost;
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
	d->fatalErrorHandler = fatalErrorHandler;
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
	d->properties.insert(Defaults::SslConfiguration, QVariant::fromValue(sslConfiguration));
	return *this;
}

Setup &Setup::setRemoteConfiguration(RemoteConfig remoteConfiguration)
{
	d->properties.insert(Defaults::RemoteConfiguration, QVariant::fromValue(remoteConfiguration));
	return *this;
}

Setup &Setup::setKeyStoreProvider(QString keyStoreProvider)
{
	d->properties.insert(Defaults::KeyStoreProvider, keyStoreProvider);
	return *this;
}

Setup &Setup::setSignatureScheme(Setup::SignatureScheme signatureScheme)
{
	d->properties.insert(Defaults::SignScheme, signatureScheme);
	return *this;
}

Setup &Setup::setSignatureKeyParam(QVariant signatureKeyParam)
{
	d->properties.insert(Defaults::SignKeyParam, signatureKeyParam);
	return *this;
}

Setup &Setup::setEncryptionScheme(Setup::EncryptionScheme encryptionScheme)
{
	d->properties.insert(Defaults::CryptScheme, encryptionScheme);
	return *this;
}

Setup &Setup::setEncryptionKeyParam(QVariant encryptionKeyParam)
{
	d->properties.insert(Defaults::CryptKeyParam, encryptionKeyParam);
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
	d->properties.insert(Defaults::SignScheme, RSA_PSS_SHA3_512);
	return *this;
}

Setup &Setup::resetSignatureKeyParam()
{
	d->properties.remove(Defaults::SignKeyParam);
	return *this;
}

Setup &Setup::resetEncryptionScheme()
{
	d->properties.insert(Defaults::CryptScheme, RSA_OAEP_SHA3_512);
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

	// create and connect the new thread
	auto thread = new QThread();
	engine->moveToThread(thread);
	QObject::connect(thread, &QThread::started,
					 engine, &ExchangeEngine::initialize);
	QObject::connect(thread, &QThread::finished,
					 engine, &ExchangeEngine::deleteLater);
	// unlock as soon as the engine has been destroyed
	QObject::connect(engine, &ExchangeEngine::destroyed, qApp, [lockFile, name](){
		qCDebug(qdssetup) << "Engine for setup" << name << "destroyed - removing lockfile";
		lockFile->unlock();
		delete lockFile;
	}, Qt::DirectConnection); //direct connection required
	// once the thread finished, clear the engine from the cache of known ones
	QObject::connect(thread, &QThread::finished, thread, [name, thread](){
		qCDebug(qdssetup) << "Thread for setup" << name << "stopped - setup completly removed";
		QMutexLocker _(&SetupPrivate::setupMutex);
		SetupPrivate::engines.remove(name);
		DefaultsPrivate::removeDefaults(name);
		thread->deleteLater();
	}, Qt::QueuedConnection); //queued connection, sent from within the thread to thread object on current thread

	// start the thread and cache engine data
	thread->start();
	SetupPrivate::engines.insert(name, {thread, engine});
}

bool Setup::createPassive(const QString &name, int timeout)
{
	QMutexLocker _(&SetupPrivate::setupMutex);

	// create storage dir and defaults
	auto storageDir = d->createStorageDir(name);
	d->createDefaults(name, storageDir, true);

	//wait for the setup to complete initialization
	auto defaults = DefaultsPrivate::obtainDefaults(name);
	auto isCreated = false;
	QEventLoop waitLoop;
	QObject::connect(defaults.data(), &DefaultsPrivate::passiveCreated,
					 &waitLoop, [&]() {
		isCreated = true;
	});
	QObject::connect(defaults.data(), &DefaultsPrivate::passiveReady,
					 &waitLoop, &QEventLoop::quit);
	QTimer::singleShot(timeout, &waitLoop, [&]() {
		waitLoop.exit(EXIT_FAILURE);
	});

	auto res = waitLoop.exec();
	Q_ASSERT_X(isCreated, Q_FUNC_INFO, "Timeout to low for the eventloop to pickup the object creation");
	return res == EXIT_SUCCESS;
}

// ------------- Private Implementation -------------

const QString SetupPrivate::DefaultLocalDir = QStringLiteral("./qtdatasync/default");
QMutex SetupPrivate::setupMutex(QMutex::Recursive);
QHash<QString, SetupPrivate::SetupInfo> SetupPrivate::engines;
unsigned long SetupPrivate::timeout = ULONG_MAX;

void SetupPrivate::cleanupHandler()
{
	QMutexLocker _(&setupMutex);
	for (auto it = engines.begin(); it != engines.end(); it++) {
		if(it->engine) {
			qCDebug(qdssetup) << "Finalizing engine of setup" << it.key() << "because of app quit";
			QMetaObject::invokeMethod(it->engine, "finalize", Qt::QueuedConnection);
			it->engine = nullptr;
		}
	}
	for (auto it = engines.constBegin(); it != engines.constEnd(); it++) {
		if(!it->thread->wait(timeout)) {
			qCWarning(qdssetup) << "Workerthread of setup" << it.key() << "did not finish before the timout. Terminating...";
			it->thread->terminate();
			auto wRes = it->thread->wait(100);
			qCDebug(qdssetup) << "Terminate result for setup" << it.key() << ":"
							  << wRes;
		}
		it->thread->deleteLater();
	}
	engines.clear();
	DefaultsPrivate::clearDefaults();
}

unsigned long SetupPrivate::currentTimeout()
{
	return timeout;
}

ExchangeEngine *SetupPrivate::engine(const QString &setupName)
{
	QMutexLocker _(&setupMutex);
	auto engine = engines.value(setupName).engine;
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

SetupPrivate::SetupPrivate() :
	localDir(DefaultLocalDir),
	roAddress(),
	serializer(new QJsonSerializer()),
	resolver(nullptr),
	properties({
		{Defaults::CacheSize, MB(100)},
		{Defaults::PersistDeleted, false},
		{Defaults::ConflictPolicy, Setup::PreferChanged},
		{Defaults::SslConfiguration, QVariant::fromValue(QSslConfiguration::defaultConfiguration())},
		{Defaults::SignScheme, Setup::RSA_PSS_SHA3_512},
		{Defaults::CryptScheme, Setup::RSA_OAEP_SHA3_512},
		{Defaults::SymScheme, Setup::AES_EAX}
	}),
	fatalErrorHandler()
{}

SetupPrivate::SetupInfo::SetupInfo() :
	SetupInfo(nullptr, nullptr)
{}

SetupPrivate::SetupInfo::SetupInfo(QThread *thread, ExchangeEngine *engine) :
	thread(thread),
	engine(engine)
{}

// ------------- Exceptions -------------

SetupException::SetupException(const QString &setupName, const QString &message) :
	Exception(setupName, message)
{}

SetupException::SetupException(const SetupException * const other) :
	Exception(other)
{}



SetupExistsException::SetupExistsException(const QString &setupName) :
	SetupException(setupName, QStringLiteral("Failed to create setup! A setup with the given name already exists!"))
{}

SetupExistsException::SetupExistsException(const SetupExistsException *const other) :
	SetupException(other)
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
	SetupException(setupName, QString()),
	_pid(-1),
	_hostname(),
	_appname()
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
	SetupException(other)
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
			   .arg(_hostname)
			   .arg(_appname);
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
