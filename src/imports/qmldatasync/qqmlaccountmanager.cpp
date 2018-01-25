#include "qqmlaccountmanager.h"
#include <QtQml>
#include <QtRemoteObjects/QRemoteObjectNode>
#include <QtDataSync/exception.h>

using namespace QtDataSync;
using std::function;

QQmlAccountManager::QQmlAccountManager(QObject *parent) :
	AccountManager(parent, nullptr),
	QQmlParserStatus(),
	_setupName(DefaultSetup),
	_node(nullptr),
	_valid(false)
{}

void QQmlAccountManager::classBegin() {}

void QQmlAccountManager::componentComplete()
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

QString QQmlAccountManager::setupName() const
{
	return _setupName;
}

QRemoteObjectNode *QQmlAccountManager::node() const
{
	return _node;
}

bool QQmlAccountManager::valid() const
{
	return _valid;
}

bool QQmlAccountManager::isTrustedImport(const QJsonObject &importData) const
{
	return AccountManager::isTrustedImport(importData);
}

void QQmlAccountManager::exportAccount(bool includeServer, QJSValue completedFn, QJSValue errorFn)
{
	if(!completedFn.isCallable())
		qmlWarning(this) << "exportAccount must be called with a function as second parameter";
	else if(!errorFn.isCallable() && !errorFn.isUndefined())
		qmlWarning(this) << "exportAccount must be called with a function as third parameter or with only 2 parameters";
	else {
		function<void(QString)> errFn;
		if(errorFn.isCallable()) {
			errFn = [errorFn](QString error) {
				auto fnCopy = errorFn;
				fnCopy.call({ error });
			};
		}
		AccountManager::exportAccount(includeServer, [this, completedFn](QJsonObject obj) {
			auto fnCopy = completedFn;
			auto context = QQmlEngine::contextForObject(this);
			if(context)
				fnCopy.call({ context->engine()->toScriptValue(obj) });
		}, errFn);
	}
}

void QQmlAccountManager::exportAccountTrusted(bool includeServer, const QString &password, QJSValue completedFn, QJSValue errorFn)
{
	if(!completedFn.isCallable())
		qmlWarning(this) << "exportAccountTrusted must be called with a function as third parameter";
	else if(!errorFn.isCallable() && !errorFn.isUndefined())
		qmlWarning(this) << "exportAccountTrusted must be called with a function as fourth parameter or with only 3 parameters";
	else {
		function<void(QString)> errFn;
		if(errorFn.isCallable()) {
			errFn = [errorFn](QString error) {
				auto fnCopy = errorFn;
				fnCopy.call({ error });
			};
		}
		AccountManager::exportAccountTrusted(includeServer, password, [this, completedFn](QJsonObject obj) {
			auto fnCopy = completedFn;
			auto context = QQmlEngine::contextForObject(this);
			if(context)
				fnCopy.call({ context->engine()->toScriptValue(obj) });
		}, errFn);
	}
}

void QQmlAccountManager::importAccount(const QJsonObject &importData, QJSValue completedFn, bool keepData)
{
	if(!completedFn.isCallable())
		qmlWarning(this) << "importAccount must be called with a function as second parameter";
	else {
		AccountManager::importAccount(importData, [completedFn](bool ok, QString error) {
			auto fnCopy = completedFn;
			fnCopy.call({ ok, error });
		}, keepData);
	}
}

void QQmlAccountManager::importAccountTrusted(const QJsonObject &importData, const QString &password, QJSValue completedFn, bool keepData)
{
	if(!completedFn.isCallable())
		qmlWarning(this) << "importAccountTrusted must be called with a function as third parameter";
	else {
		AccountManager::importAccountTrusted(importData, password, [completedFn](bool ok, QString error) {
			auto fnCopy = completedFn;
			fnCopy.call({ ok, error });
		}, keepData);
	}
}

void QQmlAccountManager::setSetupName(QString setupName)
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

void QQmlAccountManager::setNode(QRemoteObjectNode *node)
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
