#include "qqmlsyncmanager.h"
#include <QtQml>
#include <QtRemoteObjects/QRemoteObjectNode>
#include <QtDataSync/exception.h>

using namespace QtDataSync;

QQmlSyncManager::QQmlSyncManager(QObject *parent) :
	SyncManager(parent, nullptr),
	QQmlParserStatus(),
	_setupName(DefaultSetup),
	_node(nullptr),
	_valid(false)
{}

void QQmlSyncManager::classBegin() {}

void QQmlSyncManager::componentComplete()
{
	try {
		if(_node)
			initReplica(_node);
		else
			initReplica(_setupName);
		_valid = true;
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		_valid = false;
	}
	emit validChanged(_valid);
}

QString QQmlSyncManager::setupName() const
{
	return _setupName;
}

QRemoteObjectNode *QQmlSyncManager::node() const
{
	return _node;
}

bool QQmlSyncManager::valid() const
{
	return _valid;
}

void QQmlSyncManager::runOnDownloaded(QJSValue resultFn, bool triggerSync)
{
	if(!resultFn.isCallable())
		qmlWarning(this) << "runOnDownloaded must be called with a function as first parameter";
	else {
		SyncManager::runOnDownloaded([resultFn](SyncState state) {
			auto fnCopy = resultFn;
			fnCopy.call({ state });
		}, triggerSync);
	}
}

void QQmlSyncManager::runOnSynchronized(QJSValue resultFn, bool triggerSync)
{
	if(!resultFn.isCallable())
		qmlWarning(this) << "runOnSynchronized must be called with a function as first parameter";
	else {
		SyncManager::runOnSynchronized([resultFn](SyncState state) {
			auto fnCopy = resultFn;
			fnCopy.call({ state });
		}, triggerSync);
	}
}

void QQmlSyncManager::setSetupName(QString setupName)
{
	if(valid()) {
		qmlWarning(this) << "Cannot change setupName property after initialization";
		return;
	}

	if (_setupName == setupName)
		return;

	_setupName = setupName;
	emit setupNameChanged(_setupName);
}

void QQmlSyncManager::setNode(QRemoteObjectNode *node)
{
	if(valid()) {
		qmlWarning(this) << "Cannot change node property after initialization";
		return;
	}

	if (_node == node)
		return;

	_node = node;
	emit nodeChanged(_node);
}
