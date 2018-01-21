#include "qqmldatastoremodel.h"
#include <QtQml>
using namespace QtDataSync;

QQmlDataStoreModel::QQmlDataStoreModel(QObject *parent) :
	DataStoreModel(parent, nullptr),
	_setupName(DefaultSetup),
	_dataStore(nullptr)
{
	connect(this, &QQmlDataStoreModel::modelReset,
			this, [this]() {
		emit typeNameChanged(typeName());
	});
}

void QQmlDataStoreModel::classBegin() {}

void QQmlDataStoreModel::componentComplete()
{
	if(!_dataStore) {
		_dataStore = new QQmlDataStore(this);
		_dataStore->classBegin();
		_dataStore->setSetupName(_setupName);
		_dataStore->componentComplete();
		emit dataStoreChanged(_dataStore);
	}
	connect(_dataStore, &QQmlDataStore::validChanged,
			this, &QQmlDataStoreModel::validChanged);
	initStore(_dataStore);

	if(_dataStore->valid())
		emit validChanged(true);
}

QString QQmlDataStoreModel::setupName() const
{
	return _setupName;
}

QQmlDataStore *QQmlDataStoreModel::dataStore() const
{
	return _dataStore;
}

bool QQmlDataStoreModel::valid() const
{
	return _dataStore && _dataStore->valid();
}

QString QQmlDataStoreModel::typeName() const
{
	return QString::fromUtf8(QMetaType::typeName(typeId()));
}

void QQmlDataStoreModel::setSetupName(QString setupName)
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

void QQmlDataStoreModel::setDataStore(QQmlDataStore *dataStore)
{
	if(valid()) {
		qmlWarning(this) << "Cannot change dataStore property after initialization";
		return;
	}

	if (_dataStore == dataStore)
		return;

	_dataStore = dataStore;
	emit dataStoreChanged(_dataStore);
}

void QQmlDataStoreModel::setTypeName(QString typeName)
{
	try {
		setTypeId(QMetaType::type(typeName.toUtf8()));
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
	}
}
