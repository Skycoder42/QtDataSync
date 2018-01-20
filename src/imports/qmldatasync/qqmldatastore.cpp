#include "qqmldatastore.h"
using namespace QtDataSync;

QQmlDataStore::QQmlDataStore(QObject *parent) :
	QObject(parent),
	QQmlParserStatus(),
	_store(nullptr),
	_setupName(DefaultSetup)
{}

void QQmlDataStore::classBegin() {}

void QQmlDataStore::componentComplete()
{
	try {
		_store = new DataStore(_setupName, this);
		emit validChanged(true);
	} catch(Exception &e) {
		qCritical("qml: %s", e.what());//TODO as error property
	}
}

QString QQmlDataStore::setupName() const
{
	return _setupName;
}

bool QQmlDataStore::valid() const
{
	return _store;;
}

qint64 QQmlDataStore::count(const QString &typeName) const
{
	return _store->count(QMetaType::type(typeName.toUtf8()));
}

QStringList QQmlDataStore::keys(const QString &typeName) const
{
	return _store->keys(QMetaType::type(typeName.toUtf8()));
}

QVariantList QQmlDataStore::loadAll(const QString &typeName) const
{
	return _store->loadAll(QMetaType::type(typeName.toUtf8()));
}

QVariant QQmlDataStore::load(const QString &typeName, const QString &key) const
{
	return _store->load(QMetaType::type(typeName.toUtf8()), key);
}

void QQmlDataStore::save(const QString &typeName, QVariant value)
{
	_store->save(QMetaType::type(typeName.toUtf8()), value);
}

bool QQmlDataStore::remove(const QString &typeName, const QString &key)
{
	return _store->remove(QMetaType::type(typeName.toUtf8()), key);
}

QVariantList QQmlDataStore::search(const QString &typeName, const QString &query) const
{
	return _store->search(QMetaType::type(typeName.toUtf8()), query);
}

void QQmlDataStore::clear(const QString &typeName)
{
	return _store->clear(QMetaType::type(typeName.toUtf8()));
}

void QQmlDataStore::setSetupName(QString setupName)
{
	if (_setupName == setupName)
		return;

	_setupName = setupName;
	emit setupNameChanged(_setupName);
}
