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

QUuid DatabaseController::createIdentity()
{
	auto db = threadStore.localData().database();

	auto identity = QUuid::createUuid();

	QSqlQuery query(db);
	query.prepare(QStringLiteral("INSERT INTO users (identity) VALUES(?)"));
	query.addBindValue(identity.toString());
	if(query.exec())
		return identity;
	else {
		qCritical() << "Failed to add new user identity with error:"
					<< qPrintable(query.lastError().text());
		return {};
	}
}

bool DatabaseController::identify(const QUuid &identity)
{
	auto db = threadStore.localData().database();

	QSqlQuery query(db);
	query.prepare(QStringLiteral("SELECT EXISTS(SELECT identity FROM users WHERE identity = ?) AS exists"));
	query.addBindValue(identity.toString());
	if(query.exec()) {
		if(query.first())
			return true;
	} else {
		qCritical() << "Failed to identify user with error:"
					<< qPrintable(query.lastError().text());
	}

	return false;
}

void DatabaseController::initDatabase()
{
	auto db = threadStore.localData().database();

	if(!db.tables().contains("users")) {
		QSqlQuery createUsers(db);
		if(!createUsers.exec(QStringLiteral("CREATE TABLE users ( "
											"	identity	UUID PRIMARY KEY NOT NULL UNIQUE "
											");"))) {
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
											  ");"))) {
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
										   "	data	JSON, "
										   "	CONSTRAINT data_id UNIQUE (type, key)"
										   ");"))) {
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
											 ");"))) {
			qCritical() << "Failed to create states table with error:"
						<< qPrintable(createStates.lastError().text());
			return;
		}
	}
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
