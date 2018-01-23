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
				this, SLOT(dataChangedImpl(QObject*,QtDataSync::ObjectKey,bool,QJsonObject,int)),
				Qt::QueuedConnection);
		connect(_emitterBackend, SIGNAL(dataResetted(QObject*,QByteArray)),
				this, SLOT(dataResettedImpl(QObject*,QByteArray)),
				Qt::QueuedConnection);
	} else {
		connect(_emitterBackend, SIGNAL(remoteDataChanged(QtDataSync::ObjectKey,bool)),
				this, SLOT(remoteDataChangedImpl(QtDataSync::ObjectKey,bool)),
				Qt::QueuedConnection);
		connect(_emitterBackend, SIGNAL(remoteDataResetted(QByteArray)),
				this, SLOT(remoteDataResettedImpl(QByteArray)),
				Qt::QueuedConnection);
	}
}

void EmitterAdapter::triggerChange(const ObjectKey &key, const QJsonObject data, int size, bool changed)
{
	if(_isPrimary) {
		QMetaObject::invokeMethod(_emitterBackend, "triggerChange",
								  Qt::QueuedConnection,
								  Q_ARG(QObject*, parent()),
								  Q_ARG(QtDataSync::ObjectKey, key),
								  Q_ARG(QJsonObject, data),
								  Q_ARG(int, size),
								  Q_ARG(bool, changed));
		emit dataChanged(key, data.isEmpty(), data, size, true);//own change
	} else {
		QMetaObject::invokeMethod(_emitterBackend, "triggerRemoteChange",
								  Qt::QueuedConnection,
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
								  Qt::QueuedConnection,
								  Q_ARG(QObject*, parent()),
								  Q_ARG(QByteArray, typeName));
		emit dataResetted(typeName, true);
	} else {
		QMetaObject::invokeMethod(_emitterBackend, "triggerRemoteClear",
								  Qt::QueuedConnection,
								  Q_ARG(QByteArray, typeName));
		//no change signal, because operating in passive setup
	}
}

void EmitterAdapter::triggerReset()
{
	if(_isPrimary) {
		QMetaObject::invokeMethod(_emitterBackend, "triggerReset",
								  Qt::QueuedConnection,
								  Q_ARG(QObject*, parent()));
		emit dataResetted({}, true);
	} else {
		QMetaObject::invokeMethod(_emitterBackend, "triggerRemoteReset",
								  Qt::QueuedConnection);
		//no change signal, because operating in passive setup
	}
}

void EmitterAdapter::triggerUpload()
{
	QMetaObject::invokeMethod(_emitterBackend, "triggerUpload",
							  Qt::QueuedConnection);
}

void EmitterAdapter::dataChangedImpl(QObject *origin, const ObjectKey &key, bool deleted, const QJsonObject data, int size)
{
	if(origin == nullptr || origin != parent())
		emit dataChanged(key, deleted, data, size, false);
}

void EmitterAdapter::dataResettedImpl(QObject *origin, const QByteArray &typeName)
{
	if(origin == nullptr || origin != parent())
		emit dataResetted(typeName, false);
}

void EmitterAdapter::remoteDataChangedImpl(const ObjectKey &key, bool deleted)
{
	emit dataChanged(key, deleted, {}, 0, false);
}

void EmitterAdapter::remoteDataResettedImpl(const QByteArray &typeName)
{
	emit dataResetted(typeName, false);
}
