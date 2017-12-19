#include "changecontroller_p.h"
#include "datastore.h"
#include "setup_p.h"
#include "exchangeengine_p.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

using namespace QtDataSync;

QReadWriteLock ChangeController::_initLock;
QHash<QString, bool> ChangeController::_initialized;

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

ChangeController::ChangeController(const Defaults &defaults, QObject *parent) :
	Controller("change", defaults, parent),
	_database(),
	_uploadingEnabled(false)
{}

void ChangeController::initialize()
{
	_database = defaults().aquireDatabase(this);

	QWriteLocker _(defaults().databaseLock());
	createTables(defaults(), _database, true);
}

bool ChangeController::createTables(Defaults defaults, QSqlDatabase database, bool canWrite)
{
	{
		QReadLocker _(&_initLock);
		if(_initialized[defaults.setupName()])
			return true;
	}

	if(!database.tables().contains(QStringLiteral("ChangeStore"))) {
		if(!canWrite)
			return false;

		QSqlQuery createQuery(database);
		createQuery.prepare(QStringLiteral("CREATE TABLE ChangeStore ("
										   "Id			INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
										   "Type		TEXT NOT NULL,"
										   "Key			TEXT NOT NULL,"
										   "Version		INTEGER NOT NULL,"
										   "Checksum	BLOB,"
										   "UNIQUE(Type, Key)"
										   ");"));
		exec(defaults, createQuery);
	}

	if(!database.tables().contains(QStringLiteral("ClearStore"))) {
		if(!canWrite)
			return false;

		QSqlQuery createQuery(database);
		createQuery.prepare(QStringLiteral("CREATE TABLE ClearStore ("
										   "Type		TEXT NOT NULL,"
										   "PRIMARY KEY(Type)"
										   ");"));
		exec(defaults, createQuery);
	}

	QWriteLocker _(&_initLock);
	_initialized[defaults.setupName()] = true;
	return true;
}

void ChangeController::triggerDataChange(Defaults defaults, QSqlDatabase database, const ChangeInfo &changeInfo, const QWriteLocker &)
{
	QSqlQuery changeQuery(database);
	changeQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO ChangeStore (Type, Key, Version, Checksum) VALUES(?, ?, ?, ?)"));
	changeQuery.addBindValue(changeInfo.key.typeName);
	changeQuery.addBindValue(changeInfo.key.id);
	changeQuery.addBindValue(changeInfo.version);
	changeQuery.addBindValue(changeInfo.checksum);
	exec(defaults, changeQuery, changeInfo.key);

	auto instance = SetupPrivate::engine(defaults.setupName())->changeController();
	QMetaObject::invokeMethod(instance, "changeTriggered", Qt::QueuedConnection);
}

void ChangeController::triggerDataClear(Defaults defaults, QSqlDatabase database, const QByteArray &typeName, const QWriteLocker &)
{
	QSqlQuery changeQuery(database);
	changeQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO ClearStore (Type) VALUES(?)"));
	changeQuery.addBindValue(typeName);
	exec(defaults, changeQuery, typeName);

	//remove all changes stored for that type
	QSqlQuery clearQuery(database);
	clearQuery.prepare(QStringLiteral("DELETE FROM ChangeStore WHERE Type = ?"));
	clearQuery.addBindValue(typeName);
	exec(defaults, clearQuery, typeName);

	auto instance = SetupPrivate::engine(defaults.setupName())->changeController();
	QMetaObject::invokeMethod(instance, "changeTriggered", Qt::QueuedConnection);
}

QList<ChangeController::ChangeInfo> ChangeController::loadChanges()
{
	QReadLocker _(defaults().databaseLock());

	if(!createTables())
		return {};

	QSqlQuery loadQuery(_database);
	loadQuery.prepare(QStringLiteral("SELECT Type, Key, Version, Checksum FROM ChangeStore"));
	exec(loadQuery);

	QList<ChangeInfo> result;
	while(loadQuery.next()) {
		ChangeInfo info;
		info.key.typeName = loadQuery.value(0).toByteArray();
		info.key.id = loadQuery.value(1).toString();
		info.version = loadQuery.value(2).toULongLong();
		info.checksum = loadQuery.value(3).toByteArray();
		result.append(info);
	}
	return result;
}

void ChangeController::clearChanged(const ObjectKey &key, quint64 version)
{
	QWriteLocker _(defaults().databaseLock());

	if(!createTables())
		return;

	QSqlQuery clearQuery(_database);
	clearQuery.prepare(QStringLiteral("DELETE FROM ChangeStore WHERE Type = ? AND Key = ? AND version = ?"));
	clearQuery.addBindValue(key.typeName);
	clearQuery.addBindValue(key.id);
	clearQuery.addBindValue(version);
	exec(clearQuery, key);
}

QByteArrayList ChangeController::loadClears()
{
	QReadLocker _(defaults().databaseLock());

	if(!createTables())
		return {};

	QSqlQuery loadQuery(_database);
	loadQuery.prepare(QStringLiteral("SELECT Type FROM ClearStore"));
	exec(loadQuery);

	QByteArrayList result;
	while(loadQuery.next())
		result.append(loadQuery.value(0).toByteArray());
	return result;
}

void ChangeController::clearCleared(const QByteArray &typeName)
{
	QWriteLocker _(defaults().databaseLock());

	if(!createTables())
		return;

	QSqlQuery clearQuery(_database);
	clearQuery.prepare(QStringLiteral("DELETE FROM ClearStore WHERE Type = ?"));
	clearQuery.addBindValue(typeName);
	exec(clearQuery, typeName);
}

void ChangeController::setUploadingEnabled(bool uploading)
{
	_uploadingEnabled = uploading;
	//TODO check if changes ar available
	emit uploadingChanged(false);
}

void ChangeController::changeTriggered()
{
	if(_uploadingEnabled) {
		emit uploadingChanged(true);
		//TODO implement
	}
}

void ChangeController::exec(QSqlQuery &query, const ObjectKey &key) const
{
	exec(defaults(), query, key);
}

void ChangeController::exec(Defaults defaults, QSqlQuery &query, const ObjectKey &key)
{
	if(!query.exec()) {
		throw LocalStoreException(defaults,
								  key,
								  query.executedQuery().simplified(),
								  query.lastError().text());
	}
}

bool ChangeController::createTables()
{
	return createTables(defaults(), _database, false);
}



ChangeController::ChangeInfo::ChangeInfo() :
	key(),
	version(0),
	checksum()
{}

ChangeController::ChangeInfo::ChangeInfo(const ObjectKey &key, quint64 version, const QByteArray &checksum) :
	key(key),
	version(version),
	checksum(checksum)
{}
