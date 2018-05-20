#include "qqmldatastore.h"
#include <QtQml>
using namespace QtDataSync;

QQmlDataStore::QQmlDataStore(QObject *parent) :
	DataStore(parent, nullptr),
	QQmlParserStatus(),
	_setupName(DefaultSetup),
	_valid(false)
{}

void QQmlDataStore::classBegin() {}

void QQmlDataStore::componentComplete()
{
	try {
		initStore(_setupName);
		_valid = true;
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		_valid = false;
	}
	emit validChanged(_valid);
}

QString QQmlDataStore::setupName() const
{
	return _setupName;
}

bool QQmlDataStore::valid() const
{
	return _valid;
}

qint64 QQmlDataStore::count(const QString &typeName) const
{
	try {
		return DataStore::count(QMetaType::type(typeName.toUtf8()));
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return 0;
	}
}

QStringList QQmlDataStore::keys(const QString &typeName) const
{
	try {
		return DataStore::keys(QMetaType::type(typeName.toUtf8()));
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return {};
	}
}

QVariantList QQmlDataStore::loadAll(const QString &typeName) const
{
	try {
		return DataStore::loadAll(QMetaType::type(typeName.toUtf8()));
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return {};
	}
}

QVariant QQmlDataStore::load(const QString &typeName, const QString &key) const
{
	try {
		return DataStore::load(QMetaType::type(typeName.toUtf8()), key);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return {};
	}
}

void QQmlDataStore::save(const QString &typeName, const QVariant &value)
{
	try {
		DataStore::save(QMetaType::type(typeName.toUtf8()), value);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
	}
}

bool QQmlDataStore::remove(const QString &typeName, const QString &key)
{
	try {
		return DataStore::remove(QMetaType::type(typeName.toUtf8()), key);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return false;
	}
}

QVariantList QQmlDataStore::search(const QString &typeName, const QString &query, DataStore::SearchMode mode) const
{
	try {
		return DataStore::search(QMetaType::type(typeName.toUtf8()), query, mode);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return {};
	}
}

void QQmlDataStore::clear(const QString &typeName)
{
	try {
		DataStore::clear(QMetaType::type(typeName.toUtf8()));
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
	}
}

QString QQmlDataStore::typeName(int typeId) const
{
	return QString::fromUtf8(QMetaType::typeName(typeId));
}

void QQmlDataStore::setSetupName(QString setupName)
{
	if(valid()) {
		qmlWarning(this) << "Cannot change setupName property after initialization";
		return;
	}

	if (_setupName == setupName)
		return;

	_setupName = std::move(setupName);
	emit setupNameChanged(_setupName);
}
