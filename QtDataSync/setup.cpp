#include "setup.h"
#include "setup_p.h"
using namespace QtDataSync;

Setup::Setup() :
	QObject(),
	d(new SetupPrivate(this))
{}

Setup::~Setup(){}

Setup *Setup::createSetup(const QString &name)
{
	QMutexLocker _(&SetupPrivate::setupMutex);
	if(SetupPrivate::setups.contains(name))
		return SetupPrivate::setups.value(name);
	else {
		auto setup = new Setup();
		SetupPrivate::setups.insert(name, setup);
		return setup;
	}
}

Setup *Setup::setup(const QString &name)
{
	QMutexLocker _(&SetupPrivate::setupMutex);
	return SetupPrivate::setups.value(name, nullptr);
}

void Setup::deleteSetup(const QString &name)
{
	QMutexLocker _(&SetupPrivate::setupMutex);
	auto setup = SetupPrivate::setups.take(name);
	if(setup)
		QMetaObject::invokeMethod(setup, "deleteLater");
}

QJsonSerializer *Setup::serializer() const
{
	return d->serializer;
}

void Setup::setSerializer(QJsonSerializer *serializer)
{
	d->serializer = serializer;
}

// ------------- Private Implementation -------------

QMutex SetupPrivate::setupMutex;
QHash<QString, Setup*> SetupPrivate::setups;

SetupPrivate::SetupPrivate(Setup *parent) :
	serializer(new QJsonSerializer(parent))
{}
