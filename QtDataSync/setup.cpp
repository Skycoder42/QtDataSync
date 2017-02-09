#include "setup.h"
#include "setup_p.h"
#include "sqllocalstore.h"
#include "sqlstateholder.h"
#include <QCoreApplication>
using namespace QtDataSync;

static void initCleanupHandlers();
Q_COREAPP_STARTUP_FUNCTION(initCleanupHandlers)

const QString Setup::DefaultSetup(QStringLiteral("DefaultSetup"));

void Setup::setCleanupTimeout(unsigned long timeout)
{
	SetupPrivate::timeout = timeout;
}

void Setup::removeSetup(const QString &name)
{
	QMutexLocker _(&SetupPrivate::setupMutex);
	if(SetupPrivate::engines.contains(name)) {
		auto &info = SetupPrivate::engines[name];
		if(info.second) {
			QMetaObject::invokeMethod(info.second, "finalize", Qt::QueuedConnection);
			info.second = nullptr;
		}
	}
}

Setup::Setup() :
	d(new SetupPrivate())
{}

Setup::~Setup(){}

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

void Setup::create(const QString &name)
{
	QMutexLocker _(&SetupPrivate::setupMutex);
	if(SetupPrivate::engines.contains(name)) {
		qCritical() << "Failed to create setup! A setup with the name"
					<< name
					<< "already exists!";
		return;
	}

	auto engine = new StorageEngine(d->serializer.take(),
									d->localStore.take(),
									d->stateHolder.take());

	auto thread = new QThread();
	engine->moveToThread(thread);
	QObject::connect(thread, &QThread::started,
					 engine, &StorageEngine::initialize);
	QObject::connect(thread, &QThread::finished,
					 engine, &StorageEngine::deleteLater);
	QObject::connect(thread, &QThread::finished, thread, [=](){
		QMutexLocker _(&SetupPrivate::setupMutex);
		SetupPrivate::engines.remove(name);
		thread->deleteLater();
	}, Qt::QueuedConnection);
	thread->start();
	SetupPrivate::engines.insert(name, {thread, engine});
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
	serializer(new QJsonSerializer()),
	localStore(new SqlLocalStore()),
	stateHolder(new SqlStateHolder())
{}

// ------------- Application hooks -------------

static void initCleanupHandlers()
{
	qRegisterMetaType<QFutureInterface<QVariant>>("QFutureInterface<QVariant>");
	qRegisterMetaType<StorageEngine::TaskType>();
	qAddPostRoutine(SetupPrivate::cleanupHandler);
}
