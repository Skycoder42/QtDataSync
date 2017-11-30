#include "defaults.h"
#include "setup.h"
#include "setup_p.h"
#include "defaults_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QLockFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QThreadStorage>

using namespace QtDataSync;

static void initCleanupHandlers();
Q_COREAPP_STARTUP_FUNCTION(initCleanupHandlers)

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
			QMetaObject::invokeMethod(info.engine, "finalize", Qt::QueuedConnection);
			info.engine = nullptr;
		}

		if(waitForFinished) {
			if(!info.thread->wait(SetupPrivate::timeout)) {
				info.thread->terminate();
				info.thread->wait(100);
			}
			info.thread->deleteLater();
			QCoreApplication::processEvents();//required to perform queued events
		}
	}
}

Setup::Setup() :
	d(new SetupPrivate())
{
	d->serializer->setAllowDefaultNull(true);//support null gadgets
}

Setup::~Setup(){}

QString Setup::localDir() const
{
	return d->localDir;
}

QJsonSerializer *Setup::serializer() const
{
	return d->serializer.data();
}

std::function<void (QString, QString)> Setup::fatalErrorHandler() const
{
	return d->fatalErrorHandler;
}

Setup &Setup::setLocalDir(QString localDir)
{
	d->localDir = localDir;
	return *this;
}

Setup &Setup::setSerializer(QJsonSerializer *serializer)
{
	d->serializer.reset(serializer);
	return *this;
}

Setup &Setup::setFatalErrorHandler(const std::function<void (QString, QString)> &fatalErrorHandler)
{
	d->fatalErrorHandler = fatalErrorHandler;
	return *this;
}

void Setup::create(const QString &name)
{
	QMutexLocker _(&SetupPrivate::setupMutex);
	if(SetupPrivate::engines.contains(name))
		throw SetupExistsException(name);

	QDir storageDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	if(!storageDir.cd(d->localDir)) {
		storageDir.mkpath(d->localDir);
		storageDir.cd(d->localDir);
		QFile::setPermissions(storageDir.absolutePath(),
							  QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser);
	}

	auto lockFile = new QLockFile(storageDir.absoluteFilePath(QStringLiteral(".lock")));
	if(!lockFile->tryLock())
		throw SetupLockedException(lockFile, name);

	//create defaults
	DefaultsPrivate::createDefaults(name, storageDir, d->properties, d->serializer.take());

	auto engine = new ExchangeEngine(name);

	auto thread = new QThread();
	engine->moveToThread(thread);
	QObject::connect(thread, &QThread::started,
					 engine, &ExchangeEngine::initialize);
	QObject::connect(thread, &QThread::finished,
					 engine, &ExchangeEngine::deleteLater);
	QObject::connect(engine, &ExchangeEngine::destroyed, qApp, [lockFile](){
		lockFile->unlock();
		delete lockFile;
	}, Qt::DirectConnection);
	QObject::connect(thread, &QThread::finished, thread, [name, thread](){
		QMutexLocker _(&SetupPrivate::setupMutex);
		SetupPrivate::engines.remove(name);
		DefaultsPrivate::removeDefaults(name);
		thread->deleteLater();
	}, Qt::QueuedConnection);
	thread->start();
	SetupPrivate::engines.insert(name, {thread, engine});
}

int Setup::cacheSize() const
{
	return d->properties.value(Defaults::CacheSize, MB(10)).toInt();
}

void Setup::setCacheSize(int cacheSize)
{
	d->properties.insert(Defaults::CacheSize, cacheSize);
}

void Setup::resetCacheSize()
{
	d->properties.insert(Defaults::CacheSize, MB(10));
}

// ------------- Private Implementation -------------

QMutex SetupPrivate::setupMutex(QMutex::Recursive);
QHash<QString, SetupPrivate::SetupInfo> SetupPrivate::engines;
unsigned long SetupPrivate::timeout = ULONG_MAX;

void SetupPrivate::cleanupHandler()
{
	QMutexLocker _(&setupMutex);
	foreach (auto info, engines) {
		if(info.engine) {
			QMetaObject::invokeMethod(info.engine, "finalize", Qt::QueuedConnection);
			info.engine = nullptr;
		}
	}
	foreach (auto info, engines) {
		if(!info.thread->wait(timeout)) {
			info.thread->terminate();
			info.thread->wait(100);
		}
		info.thread->deleteLater();
	}
	engines.clear();
	DefaultsPrivate::clearDefaults();
}

ExchangeEngine *SetupPrivate::engine(const QString &setupName)
{
	QMutexLocker _(&setupMutex);
	return engines.value(setupName).engine;
}

SetupPrivate::SetupPrivate() :
	localDir(QStringLiteral("./qtdatasync_localstore")),
	serializer(new QJsonSerializer()),
	properties(),
	fatalErrorHandler(QtDataSync::defaultFatalErrorHandler)
{}

SetupPrivate::SetupInfo::SetupInfo() :
	SetupInfo(nullptr, nullptr)
{}

SetupPrivate::SetupInfo::SetupInfo(QThread *thread, ExchangeEngine *engine) :
	thread(thread),
	engine(engine)
{}

void QtDataSync::defaultFatalErrorHandler(QString error, QString setupName)
{
	auto name = setupName.toUtf8();
	auto msg = error.toUtf8();
	qFatal("Unrecoverable error for \"%s\" - killing application.\nError: %s",
		   name.constData(),
		   msg.constData());
}

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

static void initCleanupHandlers()
{
	qRegisterMetaType<QThread*>();
	qAddPostRoutine(SetupPrivate::cleanupHandler);
}
