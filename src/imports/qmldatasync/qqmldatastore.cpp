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
	return DataStore::count(QMetaType::type(typeName.toUtf8()));
}

QStringList QQmlDataStore::keys(const QString &typeName) const
{
	return DataStore::keys(QMetaType::type(typeName.toUtf8()));
}

QVariantList QQmlDataStore::loadAll(const QString &typeName) const
{
	return DataStore::loadAll(QMetaType::type(typeName.toUtf8()));
}

QVariant QQmlDataStore::load(const QString &typeName, const QString &key) const
{
	return DataStore::load(QMetaType::type(typeName.toUtf8()), key);
}

void QQmlDataStore::save(const QString &typeName, QVariant value)
{
	DataStore::save(QMetaType::type(typeName.toUtf8()), value);
}

bool QQmlDataStore::remove(const QString &typeName, const QString &key)
{
	return DataStore::remove(QMetaType::type(typeName.toUtf8()), key);
}

QVariantList QQmlDataStore::search(const QString &typeName, const QString &query, DataStore::SearchMode mode) const
{
	return DataStore::search(QMetaType::type(typeName.toUtf8()), query, mode);
}

void QQmlDataStore::clear(const QString &typeName)
{
	return DataStore::clear(QMetaType::type(typeName.toUtf8()));
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

	_setupName = setupName;
	emit setupNameChanged(_setupName);
}
