#include "databasecontroller.h"
#include "app.h"

#include <QtCore/QJsonDocument>

#include <QtSql/QSqlQuery>

#include <QtConcurrent/QtConcurrentRun>
using namespace QtDataSync;

namespace {

class Query : public QSqlQuery
{
public:
	Query(QSqlDatabase db);

	void prepare(const QString &query);
	void exec();

private:
	QSqlDatabase _db;
};

}

DatabaseController::DatabaseController(QObject *parent) :
	QObject(parent),
	multiThreaded(false),
	threadStore()
{
	QtConcurrent::run(qApp->threadPool(), this, &DatabaseController::initDatabase);
}

void DatabaseController::cleanupDevices(quint64 offlineSinceDays)
{
	QtConcurrent::run(qApp->threadPool(), [this, offlineSinceDays]() {
		try {
			auto db = threadStore.localData().database();
			if(!db.transaction())
				throw DatabaseException(db);

			//TODO IMPLEMENT
			Q_UNIMPLEMENTED();

			if(db.commit())
				emit cleanupOperationDone(0);
			else
				throw DatabaseException(db);
		} catch (DatabaseException &e) {
			emit cleanupOperationDone(-1, e.errorString());
		}
	});
}

QUuid DatabaseController::addNewDevice(const QString &name, const QByteArray &signScheme, const QByteArray &signKey, const QByteArray &cryptScheme, const QByteArray &cryptKey)
{
	auto db = threadStore.localData().database();
	if(!db.transaction())
		throw DatabaseException(db);

	//create a new user
	Query createIdentityQuery(db);
	createIdentityQuery.prepare(QStringLiteral("INSERT INTO users DEFAULT VALUES "
											   "RETURNING id"));
	createIdentityQuery.exec();
	if(!createIdentityQuery.first())
		throw DatabaseException(db);
	auto userId = createIdentityQuery.value(0).toLongLong();

	//create a device entry
	auto deviceId = QUuid::createUuid();
	QSqlQuery createDeviceQuery(db);
	createDeviceQuery.prepare(QStringLiteral("INSERT INTO devices "
											 "(id, userid, name, signscheme, signkey, cryptscheme, cryptkey) "
											 "VALUES(?, ?, ?, ?, ?, ?, ?) "));
	createDeviceQuery.addBindValue(deviceId);
	createDeviceQuery.addBindValue(userId);
	createDeviceQuery.addBindValue(name);
	createDeviceQuery.addBindValue(QString::fromUtf8(signScheme));
	createDeviceQuery.addBindValue(signKey);
	createDeviceQuery.addBindValue(QString::fromUtf8(cryptScheme));
	createDeviceQuery.addBindValue(cryptKey);
	createDeviceQuery.exec();

	if(!db.commit())
		throw DatabaseException(db);

	return deviceId;
}

QtDataSync::AsymmetricCryptoInfo *DatabaseController::loadCrypto(const QUuid &deviceId, CryptoPP::RandomNumberGenerator &rng, QString &name, QObject *parent)
{
	auto db = threadStore.localData().database();

	Query loadCryptoQuery(db);
	loadCryptoQuery.prepare(QStringLiteral("SELECT signscheme, signkey, cryptscheme, cryptkey, name "
										   "FROM devices "
										   "WHERE id = ?"));
	loadCryptoQuery.addBindValue(deviceId);
	loadCryptoQuery.exec();
	if(!loadCryptoQuery.first())
		throw DatabaseException(db); //TODO throw real error

	name = loadCryptoQuery.value(4).toString();
	return new AsymmetricCryptoInfo(rng,
									loadCryptoQuery.value(0).toString().toUtf8(),
									loadCryptoQuery.value(1).toByteArray(),
									loadCryptoQuery.value(2).toString().toUtf8(),
									loadCryptoQuery.value(3).toByteArray(),
									parent);

}

void DatabaseController::updateName(const QUuid &deviceId, const QString &name)
{
	auto db = threadStore.localData().database();

	Query updateNameQuery(db);
	updateNameQuery.prepare(QStringLiteral("UPDATE devices SET name = ?"
										   "WHERE id = ?"));
	updateNameQuery.addBindValue(name);
	updateNameQuery.addBindValue(deviceId);
	updateNameQuery.exec();
}

void DatabaseController::initDatabase()
{
	auto db = threadStore.localData().database();
	if(!db.isOpen()) {
		emit databaseInitDone(false);
		return;
	}

	try {
//#define AUTO_DROP_TABLES
#ifdef AUTO_DROP_TABLES
		QSqlQuery dropQuery(db);
		if(!dropQuery.exec(QStringLiteral("DROP TABLE IF EXISTS devices, users CASCADE"))) {
			qWarning() << "Failed to drop tables with error:"
					   << qPrintable(dropQuery.lastError().text());
		} else
			qInfo() << "Dropped all existing tables";
#endif

		if(!db.tables().contains(QStringLiteral("users"))) {
			QSqlQuery createUsers(db);
			if(!createUsers.exec(QStringLiteral("CREATE TABLE users ( "
											   "	id			BIGSERIAL PRIMARY KEY NOT NULL, "
											   "	keycount	INT NOT NULL DEFAULT 0 "
											   ")"))) {
				throw DatabaseException(createUsers);
			}
		}


		if(!db.tables().contains(QStringLiteral("devices"))) {
			QSqlQuery createDevices(db);
			if(!createDevices.exec(QStringLiteral("CREATE TABLE devices ( "
												  "		id			UUID PRIMARY KEY NOT NULL, "
												  "		userid		BIGINT NOT NULL REFERENCES users(id), "
												  "		name		TEXT NOT NULL, "
												  "		signscheme	TEXT NOT NULL, "
												  "		signkey		BYTEA NOT NULL, "
												  "		cryptscheme	TEXT NOT NULL, "
												  "		cryptkey	BYTEA NOT NULL, "
												  "		lastlogin	DATE NOT NULL DEFAULT 'today' "
												  ")"))) {
				throw DatabaseException(createDevices);
			}
		}

		//TODO IMPLEMENT

		emit databaseInitDone(true);
	} catch(DatabaseException &e) {
		qCritical().noquote() << "Failed to setup database. Error:" << e.errorString();
		emit databaseInitDone(false);
	}
}



DatabaseController::DatabaseWrapper::DatabaseWrapper() :
	dbName(QUuid::createUuid().toString())
{
	auto config = qApp->configuration();
	auto db = QSqlDatabase::addDatabase(config->value(QStringLiteral("database/driver"), QStringLiteral("QPSQL")).toString(), dbName);
	db.setDatabaseName(config->value(QStringLiteral("database/name"), QCoreApplication::applicationName()).toString());
	db.setHostName(config->value(QStringLiteral("database/host"), QStringLiteral("localhost")).toString());
	db.setPort(config->value(QStringLiteral("database/port"), 5432).toInt());
	db.setUserName(config->value(QStringLiteral("database/username")).toString());
	db.setPassword(config->value(QStringLiteral("database/password")).toString());
	db.setConnectOptions(config->value(QStringLiteral("database/options")).toString());

	if(!db.open()) {
		qCritical() << "Failed to open database with error:"
					<< qPrintable(db.lastError().text());
	} else
		qDebug() << "DB connected for thread" << QThread::currentThreadId();
}

DatabaseController::DatabaseWrapper::~DatabaseWrapper()
{
	QSqlDatabase::database(dbName).close();
	QSqlDatabase::removeDatabase(dbName);
	qDebug() << "DB disconnected for thread" << QThread::currentThreadId();
}

QSqlDatabase DatabaseController::DatabaseWrapper::database() const
{
	return QSqlDatabase::database(dbName);
}



DatabaseException::DatabaseException(const QSqlError &error) :
	_error(error),
	_msg(error.text().toUtf8())
{}

DatabaseException::DatabaseException(QSqlDatabase db) :
	DatabaseException(db.lastError())
{
	db.rollback(); //to be safe
}

DatabaseException::DatabaseException(const QSqlQuery &query, QSqlDatabase db) :
	DatabaseException(query.lastError())
{
	if(db.isValid())
		db.rollback(); //to be safe
}

QSqlError DatabaseException::error() const
{
	return _error;
}

QString DatabaseException::errorString() const
{
	return _error.text();
}

const char *DatabaseException::what() const noexcept
{
	return _msg.constData();
}

void DatabaseException::raise() const
{
	throw (*this);
}

QException *DatabaseException::clone() const
{
	return new DatabaseException(_error);
}



Query::Query(QSqlDatabase db) :
	QSqlQuery(db)
{}

void Query::prepare(const QString &query)
{
	if(!QSqlQuery::prepare(query))
		throw DatabaseException(*this, _db);
}

void Query::exec()
{
	if(!QSqlQuery::exec())
		throw DatabaseException(*this, _db);
}
