#include "defaults.h"
#include "exceptions.h"
#include "setup.h"
#include "setup_p.h"
#include "sqllocalstore_p.h"
#include "sqlstateholder_p.h"
#include "wsremoteconnector_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QLockFile>
#include <QtCore/QStandardPaths>

using namespace QtDataSync;

static void initCleanupHandlers();
Q_COREAPP_STARTUP_FUNCTION(initCleanupHandlers)

const QString Setup::DefaultSetup(QStringLiteral("DefaultSetup"));

void Setup::setCleanupTimeout(unsigned long timeout)
{
	SetupPrivate::timeout = timeout;
}

void Setup::removeSetup(const QString &name, bool waitForFinished)
{
	QMutexLocker _(&SetupPrivate::setupMutex);
	if(SetupPrivate::engines.contains(name)) {
		auto &info = SetupPrivate::engines[name];
		if(info.second) {
			QMetaObject::invokeMethod(info.second, "finalize", Qt::QueuedConnection);
			info.second = nullptr;
		}

		if(waitForFinished) {
			if(!info.first->wait(SetupPrivate::timeout)) {
				info.first->terminate();
				info.first->wait(100);
			}
			info.first->deleteLater();
			QCoreApplication::processEvents();//required to perform queue events
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

LocalStore *Setup::localStore() const
{
	return d->localStore.data();
}

StateHolder *Setup::stateHolder() const
{
	return d->stateHolder.data();
}

RemoteConnector *Setup::remoteConnector() const
{
	return d->remoteConnector.data();
}

DataMerger *Setup::dataMerger() const
{
	return d->dataMerger.data();
}

QVariant Setup::property(const QByteArray &key) const
{
	return d->properties.value(key);
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

Setup &Setup::setLocalStore(LocalStore *localStore)
{
	d->localStore.reset(localStore);
	return *this;
}

Setup &Setup::setStateHolder(StateHolder *stateHolder)
{
	d->stateHolder.reset(stateHolder);
	return *this;
}

Setup &Setup::setRemoteConnector(RemoteConnector *remoteConnector)
{
	d->remoteConnector.reset(remoteConnector);
	return *this;
}

Setup &Setup::setDataMerger(DataMerger *dataMerger)
{
	d->dataMerger.reset(dataMerger);
	return *this;
}

Setup &Setup::setProperty(const QByteArray &key, const QVariant &data)
{
	d->properties.insert(key, data);
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
		throw SetupLockedException(name);

	auto defaults = new Defaults(name, storageDir, d->properties);

	auto engine = new StorageEngine(defaults,
									d->serializer.take(),
									d->localStore.take(),
									d->stateHolder.take(),
									d->remoteConnector.take(),
									d->dataMerger.take());

	auto thread = new QThread();
	engine->moveToThread(thread);
	QObject::connect(thread, &QThread::started,
					 engine, &StorageEngine::initialize);
	QObject::connect(thread, &QThread::finished,
					 engine, &StorageEngine::deleteLater);
	QObject::connect(engine, &StorageEngine::destroyed, qApp, [=](){
		auto t = storageDir.path();
		qDebug() << t;
		lockFile->unlock();
		delete lockFile;
	}, Qt::DirectConnection);
	QObject::connect(thread, &QThread::finished, thread, [=](){
		QMutexLocker _(&SetupPrivate::setupMutex);
		SetupPrivate::engines.remove(name);
		thread->deleteLater();
	}, Qt::QueuedConnection);
	thread->start();
	SetupPrivate::engines.insert(name, {thread, engine});
}

Authenticator *Setup::loadAuthenticator(QObject *parent, const QString &name)
{
	QMutexLocker _(&SetupPrivate::setupMutex);
	if(SetupPrivate::engines.contains(name)) {
		auto &info = SetupPrivate::engines[name];
		if(info.second)
			return info.second->remoteConnector->createAuthenticator(info.second->defaults, parent);
	}

	return nullptr;
}

// ------------- Private Implementation -------------

QMutex SetupPrivate::setupMutex(QMutex::Recursive);
QHash<QString, QPair<QThread*, StorageEngine*>> SetupPrivate::engines;
unsigned long SetupPrivate::timeout = ULONG_MAX;

StorageEngine *SetupPrivate::engine(const QString &name)
{
	QMutexLocker _(&setupMutex);
	return SetupPrivate::engines.value(name, {nullptr, nullptr}).second;
}

void SetupPrivate::cleanupHandler()
{
	QMutexLocker _(&setupMutex);
	foreach (auto info, engines) {
		if(info.second) {
			QMetaObject::invokeMethod(info.second, "finalize", Qt::QueuedConnection);
			info.second = nullptr;
		}
	}
	foreach (auto info, engines) {
		if(!info.first->wait(timeout)) {
			info.first->terminate();
			info.first->wait(100);
		}
		info.first->deleteLater();
	}
	engines.clear();
}

SetupPrivate::SetupPrivate() :
	localDir(QStringLiteral("./qtdatasync_localstore")),
	serializer(new QJsonSerializer()),
	localStore(new SqlLocalStore()),
	stateHolder(new SqlStateHolder()),
	remoteConnector(new WsRemoteConnector()),
	dataMerger(new DataMerger()),
	properties()
{}

// ------------- Application hooks -------------

static void initCleanupHandlers()
{
	qRegisterMetaType<QFutureInterface<QVariant>>("QFutureInterface<QVariant>");
	qRegisterMetaType<QThread*>();
	qRegisterMetaType<StorageEngine::TaskType>();
	qAddPostRoutine(SetupPrivate::cleanupHandler);
}
