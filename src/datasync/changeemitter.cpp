#include "changeemitter_p.h"
using namespace QtDataSync;

ChangeEmitter::ChangeEmitter(QObject *parent) :
	ChangeEmitterSource(parent)
{}

void ChangeEmitter::triggerChange(QObject *origin, const ObjectKey &key, bool deleted, bool changed)
{
	if(changed)
		emit uploadNeeded();
	emit dataChanged(origin, key, deleted);
	emit remoteDataChanged(key, deleted);
}

void ChangeEmitter::triggerClear(QObject *origin, const QByteArray &typeName)
{
	emit uploadNeeded();
	emit dataResetted(origin, typeName);
	emit remoteDataResetted(typeName);
}

void ChangeEmitter::triggerReset(QObject *origin)
{
	emit uploadNeeded();
	emit dataResetted(origin, {});
	emit remoteDataResetted({});
}

void ChangeEmitter::triggerUpload()
{
	emit uploadNeeded();
}

void ChangeEmitter::triggerRemoteChange(const ObjectKey &key, bool deleted, bool changed)
{
	if(changed)
		emit uploadNeeded();
	emit dataChanged(nullptr, key, deleted);
	emit remoteDataChanged(key, deleted);
}

void ChangeEmitter::triggerRemoteClear(const QByteArray &typeName)
{
	emit uploadNeeded();
	emit dataResetted(nullptr, typeName);
	emit remoteDataResetted(typeName);
}

void ChangeEmitter::triggerRemoteReset()
{
	emit uploadNeeded();
	emit dataResetted(nullptr, {});
	emit remoteDataResetted({});
}
