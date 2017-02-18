#include "databasecontroller.h"
#include "app.h"
#include <QtSql>
#include <QtConcurrent>

DatabaseController::DatabaseController(QObject *parent) :
	QObject(parent),
	multiThreaded(false),
	threadStore()
{
	QtConcurrent::run(this, &DatabaseController::initDatabase);
}

QUuid DatabaseController::createIdentity(const QUuid &deviceId)
{
	auto db = threadStore.localData().database();
	if(!db.transaction()) {
		qCritical() << "Failed to create transaction with error:"
					<< qPrintable(db.lastError().text());
		return {};
	}

	auto identity = QUuid::createUuid();

	QSqlQuery createIdentityQuery(db);
	createIdentityQuery.prepare(QStringLiteral("INSERT INTO users (identity) VALUES(?)"));
	createIdentityQuery.addBindValue(identity);
	if(!createIdentityQuery.exec()) {
		qCritical() << "Failed to add new user identity with error:"
					<< qPrintable(createIdentityQuery.lastError().text());
		db.rollback();
		return {};
	}

	QSqlQuery createDeviceQuery(db);
	createDeviceQuery.prepare(QStringLiteral("INSERT INTO devices (deviceid, userid) VALUES(?, ?)"));
	createDeviceQuery.addBindValue(deviceId);
	createDeviceQuery.addBindValue(identity);
	if(!createDeviceQuery.exec()) {
		qCritical() << "Failed to add new device with error:"
					<< qPrintable(createDeviceQuery.lastError().text());
		db.rollback();
		return {};
	}

	if(db.commit())
		return identity;
	else {
		qCritical() << "Failed to commit transaction with error:"
					<< qPrintable(db.lastError().text());
		return {};
	}
}

bool DatabaseController::identify(const QUuid &identity, const QUuid &deviceId)
{
	auto db = threadStore.localData().database();

	QSqlQuery identifyQuery(db);
	identifyQuery.prepare(QStringLiteral("SELECT EXISTS(SELECT identity FROM users WHERE identity = ?) AS exists"));
	identifyQuery.addBindValue(identity);
	if(!identifyQuery.exec() || !identifyQuery.first()) {
		qCritical() << "Failed to identify user with error:"
					<< qPrintable(identifyQuery.lastError().text());
		return false;
	}

	if(identifyQuery.value(0).toBool()) {
		QSqlQuery createDeviceQuery(db);
		createDeviceQuery.prepare(QStringLiteral("INSERT INTO devices (deviceid, userid) VALUES(?, ?) "
												 "ON CONFLICT DO NOTHING"));
		createDeviceQuery.addBindValue(deviceId);
		createDeviceQuery.addBindValue(identity);
		if(!createDeviceQuery.exec()) {
			qCritical() << "Failed to add (new) device with error:"
						<< qPrintable(createDeviceQuery.lastError().text());
			return false;
		} else
			return true;
	} else
		return false;
}

QJsonValue DatabaseController::load(const QUuid &userId, const QString &type, const QString &key)
{
	auto db = threadStore.localData().database();

	QSqlQuery loadQuery(db);
	loadQuery.prepare(QStringLiteral("SELECT data FROM data WHERE userid = ? AND type = ? AND key = ?"));
	loadQuery.addBindValue(userId);
	loadQuery.addBindValue(type);
	loadQuery.addBindValue(key);
	if(!loadQuery.exec()) {
		qCritical() << "Failed to load data with error:"
					<< qPrintable(loadQuery.lastError().text());
		return QJsonValue::Null;
	}

	if(loadQuery.first())
		return stringToJson(loadQuery.value(0).toString());
	else
		return QJsonValue::Null;
}

bool DatabaseController::save(const QUuid &userId, const QUuid &deviceId, const QString &type, const QString &key, const QJsonObject &object)
{
	auto db = threadStore.localData().database();
	if(!db.transaction()) {
		qCritical() << "Failed to create transaction with error:"
					<< qPrintable(db.lastError().text());
		return false;
	}

	// insert/update the data
	QSqlQuery saveQuery(db);
	saveQuery.prepare(QStringLiteral("INSERT INTO data (userid, type, key, data) VALUES(?, ?, ?, ?) "
									 "ON CONFLICT (userid, type, key) DO UPDATE "
									 "SET data = EXCLUDED.data "
									 "RETURNING index"));
	saveQuery.addBindValue(userId);
	saveQuery.addBindValue(type);
	saveQuery.addBindValue(key);
	saveQuery.addBindValue(jsonToString(object));
	if(!saveQuery.exec() || !saveQuery.first()) {
		qCritical() << "Failed to insert/update data with error:"
					<< qPrintable(saveQuery.lastError().text());
		db.rollback();
		return false;
	}
	auto index = saveQuery.value(0).toULongLong();

	//update the change state
	if(!updateDeviceStates(db, userId, deviceId, index)) {
		db.rollback();
		return false;
	}

	//TODO notify all connected devices

	if(db.commit())
		return true;
	else {
		qCritical() << "Failed to commit transaction with error:"
					<< qPrintable(db.lastError().text());
		return false;
	}
}

bool DatabaseController::remove(const QUuid &userId, const QUuid &deviceId, const QString &type, const QString &key)
{
	auto db = threadStore.localData().database();
	if(!db.transaction()) {
		qCritical() << "Failed to create transaction with error:"
					<< qPrintable(db.lastError().text());
		return false;
	}

	// insert/update the data
	QSqlQuery removeQuery(db);
	removeQuery.prepare(QStringLiteral("INSERT INTO data (userid, type, key, data) VALUES(?, ?, ?, NULL) "
									   "ON CONFLICT (userid, type, key) DO UPDATE "
									   "SET data = NULL "
									   "RETURNING index"));
	removeQuery.addBindValue(userId);
	removeQuery.addBindValue(type);
	removeQuery.addBindValue(key);
	if(!removeQuery.exec() || !removeQuery.first()) {
		qCritical() << "Failed to insert/update data with error:"
					<< qPrintable(removeQuery.lastError().text());
		db.rollback();
		return false;
	}
	auto index = removeQuery.value(0).toULongLong();

	//update the change state
	if(!updateDeviceStates(db, userId, deviceId, index)) {
		db.rollback();
		return false;
	}

	//TODO notify all connected devices

	if(db.commit()){
		tryDeleteData(db, index);//try to delete AFTER commiting
		return true;
	}
	else {
		qCritical() << "Failed to commit transaction with error:"
					<< qPrintable(db.lastError().text());
		return false;
	}
}

bool DatabaseController::markUnchanged(const QUuid &userId, const QUuid &deviceId, const QString &type, const QString &key)
{
	auto db = threadStore.localData().database();
	if(!db.transaction()) {
		qCritical() << "Failed to create transaction with error:"
					<< qPrintable(db.lastError().text());
		return false;
	}

	//get the data index
	QSqlQuery dataIndexQuery(db);
	dataIndexQuery.prepare(QStringLiteral("SELECT index FROM data WHERE userid = ? AND type = ? AND key = ?"));
	dataIndexQuery.addBindValue(userId);
	dataIndexQuery.addBindValue(type);
	dataIndexQuery.addBindValue(key);
	if(!dataIndexQuery.exec()) {
		qCritical() << "Failed to get data index with error:"
					<< qPrintable(dataIndexQuery.lastError().text());
		db.rollback();
		return false;
	}

	if(!dataIndexQuery.first()) {//nothing to be deleted -> implicit success
		db.rollback();//nothing has been changed
		return true;
	}

	//mark as unchanged
	auto index = dataIndexQuery.value(0).toULongLong();
	if(!markStateUnchanged(db, userId, deviceId, index)) {
		db.rollback();
		return false;
	}

	if(db.commit()){
		tryDeleteData(db, index);//try to delete AFTER commiting
		return true;
	}
	else {
		qCritical() << "Failed to commit transaction with error:"
					<< qPrintable(db.lastError().text());
		return false;
	}
}

void DatabaseController::initDatabase()
{
	auto db = threadStore.localData().database();

	if(!db.tables().contains("users")) {
		QSqlQuery createUsers(db);
		if(!createUsers.exec(QStringLiteral("CREATE TABLE users ( "
											"	identity	UUID PRIMARY KEY NOT NULL UNIQUE "
											")"))) {
			qCritical() << "Failed to create users table with error:"
						<< qPrintable(createUsers.lastError().text());
			return;
		}
	}
	if(!db.tables().contains("devices")) {
		QSqlQuery createDevices(db);
		if(!createDevices.exec(QStringLiteral("CREATE TABLE devices ( "
											  "		id			SERIAL PRIMARY KEY NOT NULL, "
											  "		deviceid	UUID NOT NULL, "
											  "		userid		UUID NOT NULL REFERENCES users(identity), "
											  "		CONSTRAINT device_id UNIQUE (deviceid, userid)"
											  ")"))) {
			qCritical() << "Failed to create devices table with error:"
						<< qPrintable(createDevices.lastError().text());
			return;
		}
	}
	if(!db.tables().contains("data")) {
		QSqlQuery createData(db);
		if(!createData.exec(QStringLiteral("CREATE TABLE data ( "
										   "	index	SERIAL PRIMARY KEY NOT NULL, "
										   "	userid	UUID NOT NULL REFERENCES users(identity), "
										   "	type	TEXT NOT NULL, "
										   "	key		TEXT NOT NULL, "
										   "	data	JSONB, "
										   "	CONSTRAINT data_id UNIQUE (userid, type, key)"
										   ")"))) {
			qCritical() << "Failed to create data table with error:"
						<< qPrintable(createData.lastError().text());
			return;
		}
	}
	if(!db.tables().contains("states")) {
		QSqlQuery createStates(db);
		if(!createStates.exec(QStringLiteral("CREATE TABLE states ( "
											 "	dataindex	INTEGER NOT NULL REFERENCES data(index), "
											 "	deviceid	INTEGER NOT NULL REFERENCES devices(id), "
											 "	PRIMARY KEY (dataindex, deviceid)"
											 ")"))) {
			qCritical() << "Failed to create states table with error:"
						<< qPrintable(createStates.lastError().text());
			return;
		}
	}
}

bool DatabaseController::markStateUnchanged(QSqlDatabase &database, const QUuid &userId, const QUuid &deviceId, quint64 index)
{
	QSqlQuery markUnchangedQuery(database);
	markUnchangedQuery.prepare(QStringLiteral("DELETE FROM states WHERE dataindex = ? AND deviceid = ("
											  "	SELECT id FROM devices WHERE deviceid = ? AND userid = ?"
											  ")"));
	markUnchangedQuery.addBindValue(index);
	markUnchangedQuery.addBindValue(deviceId);
	markUnchangedQuery.addBindValue(userId);
	if(!markUnchangedQuery.exec()) {
		qCritical() << "Failed to mark state unchanged with error:"
					<< qPrintable(markUnchangedQuery.lastError().text());
		return false;
	} else
		return true;
}

bool DatabaseController::updateDeviceStates(QSqlDatabase &database, const QUuid &userId, const QUuid &deviceId, quint64 index)
{
	QSqlQuery updateStateQuery(database);
	updateStateQuery.prepare(QStringLiteral("INSERT INTO states (dataindex, deviceid) "
											"SELECT ?, id FROM devices "
											"WHERE userid = ? AND deviceid != ? "
											"ON CONFLICT DO NOTHING"));
	updateStateQuery.addBindValue(index);
	updateStateQuery.addBindValue(userId);
	updateStateQuery.addBindValue(deviceId);
	if(!updateStateQuery.exec()) {
		qCritical() << "Failed to update device states with error:"
					<< qPrintable(updateStateQuery.lastError().text());
		return false;
	}

	return markStateUnchanged(database, userId, deviceId, index);
}

void DatabaseController::tryDeleteData(QSqlDatabase &database, quint64 index)
{
	QSqlQuery tryDeleteQuery(database);
	tryDeleteQuery.prepare("DELETE FROM data WHERE index = ?");
	tryDeleteQuery.addBindValue(index);
	tryDeleteQuery.exec();//ignore errors -> if error, data still used -> OK
}

QString DatabaseController::jsonToString(const QJsonObject &object) const
{
	return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QJsonObject DatabaseController::stringToJson(const QString &data) const
{
	return QJsonDocument::fromJson(data.toUtf8()).object();
}



DatabaseController::DatabaseWrapper::DatabaseWrapper() :
	dbName(QUuid::createUuid().toString())
{
	auto config = qApp->configuration();
	auto db = QSqlDatabase::addDatabase(config->value("database/driver", "QPSQL").toString(), dbName);
	db.setDatabaseName(config->value("database/name", "QtDataSync").toString());
	db.setHostName(config->value("database/host").toString());
	db.setPort(config->value("database/port").toInt());
	db.setUserName(config->value("database/username").toString());
	db.setPassword(config->value("database/password").toString());
	db.setConnectOptions(config->value("database/options").toString());

	if(!db.open()) {
		qCritical() << "Failed to open database with error:"
					<< qPrintable(db.lastError().text());
	} else
		qInfo() << "DB connected for thread" << QThread::currentThreadId();
}

DatabaseController::DatabaseWrapper::~DatabaseWrapper()
{
	QSqlDatabase::database(dbName).close();
	QSqlDatabase::removeDatabase(dbName);
	qInfo() << "DB disconnected for thread" << QThread::currentThreadId();
}

QSqlDatabase DatabaseController::DatabaseWrapper::database() const
{
	return QSqlDatabase::database(dbName);
}
