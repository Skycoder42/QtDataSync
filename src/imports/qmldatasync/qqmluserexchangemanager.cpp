#include "qqmluserexchangemanager.h"
#include <QtQml>
using namespace QtDataSync;

QQmlUserExchangeManager::QQmlUserExchangeManager(QObject *parent) :
	UserExchangeManager(parent, nullptr),
	QQmlParserStatus(),
	_setupName(DefaultSetup),
	_manager(nullptr)
{}

void QQmlUserExchangeManager::classBegin() {}

void QQmlUserExchangeManager::componentComplete()
{
	if(!_manager) {
		_manager = new QQmlAccountManager(this);
		_manager->classBegin();
		_manager->setSetupName(_setupName);
		_manager->componentComplete();
		emit managerChanged(_manager);
	}
	connect(_manager, &QQmlAccountManager::validChanged,
			this, &QQmlUserExchangeManager::validChanged);
	initManager(_manager);

	if(_manager->valid())
		emit validChanged(true);
}

quint16 QQmlUserExchangeManager::DataExchangePort() const
{
	return UserExchangeManager::DataExchangePort;
}

QString QQmlUserExchangeManager::setupName() const
{
	return _setupName;
}

QQmlAccountManager *QQmlUserExchangeManager::manager() const
{
	return _manager;
}

bool QQmlUserExchangeManager::valid() const
{
	return _manager && _manager->valid();
}

void QQmlUserExchangeManager::importFrom(const UserInfo &userInfo, QJSValue completedFn, bool keepData)
{
	if(!completedFn.isCallable())
		qmlWarning(this) << "importFrom must be called with a function as second parameter";
	else {
		UserExchangeManager::importFrom(userInfo, [completedFn](bool ok, QString error) {
			auto fnCopy = completedFn;
			fnCopy.call({ ok, error });
		}, keepData);
	}
}

void QQmlUserExchangeManager::importTrustedFrom(const UserInfo &userInfo, const QString &password, QJSValue completedFn, bool keepData)
{
	if(!completedFn.isCallable())
		qmlWarning(this) << "importTrustedFrom must be called with a function as third parameter";
	else {
		UserExchangeManager::importTrustedFrom(userInfo, password, [completedFn](bool ok, QString error) {
			auto fnCopy = completedFn;
			fnCopy.call({ ok, error });
		}, keepData);
	}
}

void QQmlUserExchangeManager::setSetupName(QString setupName)
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

void QQmlUserExchangeManager::setManager(QQmlAccountManager *manager)
{
	if(valid()) {
		qmlWarning(this) << "Cannot change manager property after initialization";
		return;
	}

	if (_manager == manager)
		return;

	_manager = manager;
	emit managerChanged(_manager);
}
