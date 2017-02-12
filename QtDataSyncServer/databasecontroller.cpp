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
		query.prepare(QStringLiteral("INSERT INTO Users (Identity) VALUES(?)"));
		query.addBindValue(identity.toString());
		if(query.exec()) {
			QMetaObject::invokeMethod(object, method.constData(), Q_ARG(QUuid, identity));
			return;
		} else {
			qCritical() << "Failed to add new user identity with error:"
						<< query.lastError().text();
			QMetaObject::invokeMethod(object, method.constData(), Q_ARG(QUuid, {}));
		}
	});
}

void DatabaseController::identify(const QUuid &identity, QObject *object, const QByteArray &method)
{
	QtConcurrent::run(pool, [=](){
		auto db = threadStore.localData().database();

		QSqlQuery query(db);
		query.prepare(QStringLiteral("SELECT Identity FROM Users WHERE Identity = ?"));
		query.addBindValue(identity.toString());
		if(query.exec()) {
			if(query.first()) {
				QMetaObject::invokeMethod(object, method.constData(), Q_ARG(bool, true));
				return;
			}
		} else {
			qCritical() << "Failed to identify user with error:"
						<< query.lastError().text();
		}

		QMetaObject::invokeMethod(object, method.constData(), Q_ARG(bool, false));
	});
}



DatabaseController::DatabaseWrapper::DatabaseWrapper() :
	dbName(QUuid::createUuid().toString())
{
	auto config = qApp->configuration();
	auto db = QSqlDatabase::addDatabase(config->value("database/driver", "QSQLITE").toString(), dbName);
	db.setDatabaseName(qApp->absolutePath(config->value("database/name", "./data.db").toString()));
	db.setHostName(config->value("database/host").toString());
	db.setPort(config->value("database/port").toInt());
	db.setUserName(config->value("database/username").toString());
	db.setPassword(config->value("database/password").toString());
	db.setConnectOptions(config->value("database/options").toString());

	if(!db.open()) {
		qCritical() << "Failed to open database with error:"
					<< db.lastError().text();
	} else if(!db.tables().contains("Users")) {
		QSqlQuery createUsers(db);
		createUsers.prepare(QStringLiteral("CREATE TABLE Users ("
										   "	Identity	TEXT NOT NULL UNIQUE,"
										   "	PRIMARY KEY(Identity)"
										   ");"));
		if(!createUsers.exec()) {
			qCritical() << "Failed to create Users table with error:"
						<< createUsers.lastError().text();
		}
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
