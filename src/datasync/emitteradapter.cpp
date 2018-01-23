#include "emitteradapter_p.h"
#include "changeemitter_p.h"
using namespace QtDataSync;

EmitterAdapter::EmitterAdapter(QObject *changeEmitter, QObject *origin) :
	QObject(origin),
	_isPrimary(changeEmitter->metaObject()->inherits(&ChangeEmitter::staticMetaObject)),
	_emitterBackend(changeEmitter)
{
	if(_isPrimary) {
		connect(_emitterBackend, SIGNAL(dataChanged(QObject*,QtDataSync::ObjectKey,bool,QJsonObject,int)),
				this, SLOT(dataChangedImpl(QObject*,QtDataSync::ObjectKey,bool,QJsonObject,int)));
		connect(_emitterBackend, SIGNAL(dataResetted(QObject*,QByteArray)),
				this, SLOT(dataResettedImpl(QObject*,QByteArray)));
	} else {
		connect(_emitterBackend, SIGNAL(remoteDataChanged(QtDataSync::ObjectKey,bool)),
				this, SLOT(remoteDataChangedImpl(QtDataSync::ObjectKey,bool)));
		connect(_emitterBackend, SIGNAL(remoteDataResetted(QByteArray)),
				this, SLOT(remoteDataResettedImpl(QByteArray)));
	}
}

void EmitterAdapter::triggerChange(const ObjectKey &key, const QJsonObject data, int size, bool changed)
{
	if(_isPrimary) {
		QMetaObject::invokeMethod(_emitterBackend, "triggerChange",
								  Q_ARG(QObject*, parent()),
								  Q_ARG(QtDataSync::ObjectKey, key),
								  Q_ARG(QJsonObject, data),
								  Q_ARG(int, size),
								  Q_ARG(bool, changed));
		emit dataChanged(key, data.isEmpty(), data, size);//own change
	} else {
		QMetaObject::invokeMethod(_emitterBackend, "triggerRemoteChange",
								  Q_ARG(QtDataSync::ObjectKey, key),
								  Q_ARG(bool, data.isEmpty()),
								  Q_ARG(bool, changed));
		//no change signal, because operating in passive setup
	}
}

void EmitterAdapter::triggerClear(const QByteArray &typeName)
{
	if(_isPrimary) {
		QMetaObject::invokeMethod(_emitterBackend, "triggerClear",
								  Q_ARG(QObject*, parent()),
								  Q_ARG(QByteArray, typeName));
		emit dataResetted(typeName);
	} else {
		QMetaObject::invokeMethod(_emitterBackend, "triggerRemoteClear",
								  Q_ARG(QByteArray, typeName));
		//no change signal, because operating in passive setup
	}
}

void EmitterAdapter::triggerReset()
{
	if(_isPrimary) {
		QMetaObject::invokeMethod(_emitterBackend, "triggerReset",
								  Q_ARG(QObject*, parent()));
		emit dataResetted({});
	} else {
		QMetaObject::invokeMethod(_emitterBackend, "triggerRemoteReset");
		//no change signal, because operating in passive setup
	}
}

void EmitterAdapter::dataChangedImpl(QObject *origin, const ObjectKey &key, bool deleted, const QJsonObject data, int size)
{
	if(origin == nullptr || origin != parent())
		emit dataChanged(key, deleted, data, size);
}

void EmitterAdapter::dataResettedImpl(QObject *origin, const QByteArray &typeName)
{
	if(origin == nullptr || origin != parent())
		emit dataResetted(typeName);
}

void EmitterAdapter::remoteDataChangedImpl(const ObjectKey &key, bool deleted)
{
	emit dataChanged(key, deleted, {}, 0);
}

void EmitterAdapter::remoteDataResettedImpl(const QByteArray &typeName)
{
	emit dataResetted(typeName);
}
