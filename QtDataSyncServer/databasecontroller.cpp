#include "databasecontroller.h"
#include "app.h"
#include <QtSql>
#include <QtConcurrent>

DatabaseController::DatabaseController(QObject *parent) :
	QObject(parent),
	multiThreaded(false),
	pool(new QThreadPool(this)),
	threadStore()
{
	multiThreaded = qApp->configuration()->value("database/multithreaded", false).toBool();
	if(!multiThreaded)
		pool->setMaxThreadCount(1);//limit to 1 thread

	this->initDatabase();
}

DatabaseController::~DatabaseController()
{
	pool->clear();
	pool->waitForDone();
}

void DatabaseController::createIdentity(QObject *object, const QByteArray &method)
{
	QtConcurrent::run(pool, [=](){
		auto db = threadStore.localData().database();

		auto identity = QUuid::createUuid();

		QSqlQuery query(db);
		query.prepare(QStringLiteral("INSERT INTO users (Identity) VALUES(?)"));
		query.addBindValue(identity.toString());
		if(query.exec()) {
			QMetaObject::invokeMethod(object, method.constData(), Q_ARG(QUuid, identity));
			return;
		} else {
			qCritical() << "Failed to add new user identity with error:"
						<< qPrintable(query.lastError().text());
			QMetaObject::invokeMethod(object, method.constData(), Q_ARG(QUuid, {}));
		}
	});
}

void DatabaseController::identify(const QUuid &identity, QObject *object, const QByteArray &method)
{
	QtConcurrent::run(pool, [=](){
		auto db = threadStore.localData().database();

		QSqlQuery query(db);
		query.prepare(QStringLiteral("SELECT Identity FROM users WHERE Identity = ?"));
		query.addBindValue(identity.toString());
		if(query.exec()) {
			if(query.first()) {
				QMetaObject::invokeMethod(object, method.constData(), Q_ARG(bool, true));
				return;
			}
		} else {
			qCritical() << "Failed to identify user with error:"
						<< qPrintable(query.lastError().text());
		}

		QMetaObject::invokeMethod(object, method.constData(), Q_ARG(bool, false));
	});
}

void DatabaseController::initDatabase()
{
	QtConcurrent::run(pool, [=](){
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
	});
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
	}
}

DatabaseController::DatabaseWrapper::~DatabaseWrapper()
{
	QSqlDatabase::database(dbName).close();
	QSqlDatabase::removeDatabase(dbName);
}

QSqlDatabase DatabaseController::DatabaseWrapper::database() const
{
	return QSqlDatabase::database(dbName);
}
