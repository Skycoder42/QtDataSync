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
		qCritical("%s", e.what());//TODO as error property
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

void QQmlDataStore::setSetupName(QString setupName)
{
	if (_setupName == setupName)
		return;

	_setupName = setupName;
	emit setupNameChanged(_setupName);
}
