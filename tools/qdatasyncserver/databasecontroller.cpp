#include "databasecontroller.h"
#include "app.h"

#include <QtCore/QJsonDocument>

#include <QtSql/QSqlQuery>

#include <QtConcurrent/QtConcurrentRun>

#if QT_HAS_INCLUDE(<chrono>)
#define scdtime(x) x
#else
#define scdtime(x) std::chrono::duration_cast<std::chrono::milliseconds>(x).count()
#endif

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
	_threadStore(),
	_keepAliveTimer(nullptr)
{
	connect(this, &DatabaseController::databaseInitDone, this, [this](bool ok) {
		if(ok) { //done on the main thread to make sure the connection does not die with threads
			auto driver = _threadStore.localData().database().driver();
			connect(driver, QOverload<const QString &, QSqlDriver::NotificationSource, const QVariant &>::of(&QSqlDriver::notification),
					this, &DatabaseController::onNotify);
			if(!driver->subscribeToNotification(QStringLiteral("deviceDataEvent")))
				qCritical() << "Unabled to notify to change events. Devices will not receive updates!";
			else
				qDebug() << "Event subscription active";

			auto delay = qApp->configuration()->value(QStringLiteral("database/keepaliveInterval"), 5).toInt();
			if(delay > 0) {
				_keepAliveTimer = new QTimer(this);
				_keepAliveTimer->setInterval(scdtime(std::chrono::minutes(delay)));
				_keepAliveTimer->setTimerType(Qt::VeryCoarseTimer);
				connect(_keepAliveTimer, &QTimer::timeout,
						this, &DatabaseController::timeout);
				_keepAliveTimer->start();
			}
		}
	}, Qt::QueuedConnection);

	QtConcurrent::run(qApp->threadPool(), this, &DatabaseController::initDatabase);
}

void DatabaseController::cleanupDevices(quint64 offlineSinceDays)
{
	QtConcurrent::run(qApp->threadPool(), [this, offlineSinceDays]() {
		try {
			auto db = _threadStore.localData().database();
			if(!db.transaction())
				throw DatabaseException(db);

			try {
				//TODO IMPLEMENT
				Q_UNIMPLEMENTED();

				if(db.commit())
					emit cleanupOperationDone(0);
				else
					throw DatabaseException(db);
			} catch(...) {
				db.rollback();
				throw;
			}
		} catch (DatabaseException &e) {
			emit cleanupOperationDone(-1, e.error().text());
		}
	});
}

QUuid DatabaseController::addNewDevice(const QString &name, const QByteArray &signScheme, const QByteArray &signKey, const QByteArray &cryptScheme, const QByteArray &cryptKey, const QByteArray &fingerprint)
{
	auto db = _threadStore.localData().database();
	if(!db.transaction())
		throw DatabaseException(db);

	try {
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
		Query createDeviceQuery(db);
		createDeviceQuery.prepare(QStringLiteral("INSERT INTO devices "
												 "(id, userid, name, signscheme, signkey, cryptscheme, cryptkey, fingerprint) "
												 "VALUES(?, ?, ?, ?, ?, ?, ?, ?) "));
		createDeviceQuery.addBindValue(deviceId);
		createDeviceQuery.addBindValue(userId);
		createDeviceQuery.addBindValue(name);
		createDeviceQuery.addBindValue(QString::fromUtf8(signScheme));
		createDeviceQuery.addBindValue(signKey);
		createDeviceQuery.addBindValue(QString::fromUtf8(cryptScheme));
		createDeviceQuery.addBindValue(cryptKey);
		createDeviceQuery.addBindValue(fingerprint);
		createDeviceQuery.exec();

		if(!db.commit())
			throw DatabaseException(db);

		return deviceId;
	} catch(...) {
		db.rollback();
		throw;
	}
}

void DatabaseController::addNewDeviceToUser(const QUuid &newDeviceId, const QUuid &partnerDeviceId, const QString &name, const QByteArray &signScheme, const QByteArray &signKey, const QByteArray &cryptScheme, const QByteArray &cryptKey, const QByteArray &fingerprint)
{
	auto db = _threadStore.localData().database();

	Query createDeviceQuery(db);
	createDeviceQuery.prepare(QStringLiteral("INSERT INTO devices "
											 "(id, userid, name, signscheme, signkey, cryptscheme, cryptkey, fingerprint) "
											 "VALUES(?, ( "
											 "	SELECT userid FROM devices " //TODO make stored procedure
											 "	WHERE id = ? "
											 "), ?, ?, ?, ?, ?, ?) "));
	createDeviceQuery.addBindValue(newDeviceId);
	createDeviceQuery.addBindValue(partnerDeviceId);
	createDeviceQuery.addBindValue(name);
	createDeviceQuery.addBindValue(QString::fromUtf8(signScheme));
	createDeviceQuery.addBindValue(signKey);
	createDeviceQuery.addBindValue(QString::fromUtf8(cryptScheme));
	createDeviceQuery.addBindValue(cryptKey);
	createDeviceQuery.addBindValue(fingerprint);
	createDeviceQuery.exec();
}

AsymmetricCryptoInfo *DatabaseController::loadCrypto(const QUuid &deviceId, CryptoPP::RandomNumberGenerator &rng, QObject *parent)
{
	auto db = _threadStore.localData().database();

	Query loadCryptoQuery(db);
	loadCryptoQuery.prepare(QStringLiteral("SELECT signscheme, signkey, cryptscheme, cryptkey "
										   "FROM devices "
										   "WHERE id = ?"));
	loadCryptoQuery.addBindValue(deviceId);
	loadCryptoQuery.exec();
	if(!loadCryptoQuery.first())
		return nullptr;

	return new AsymmetricCryptoInfo(rng,
									loadCryptoQuery.value(0).toString().toUtf8(),
									loadCryptoQuery.value(1).toByteArray(),
									loadCryptoQuery.value(2).toString().toUtf8(),
									loadCryptoQuery.value(3).toByteArray(),
									parent);
}

void DatabaseController::updateLogin(const QUuid &deviceId, const QString &name)
{
	auto db = _threadStore.localData().database();

	Query updateNameQuery(db);
	updateNameQuery.prepare(QStringLiteral("UPDATE devices SET name = ?, lastlogin = 'today'"
										   "WHERE id = ?"));
	updateNameQuery.addBindValue(name);
	updateNameQuery.addBindValue(deviceId);
	updateNameQuery.exec();
}

QList<std::tuple<QUuid, QString, QByteArray>> DatabaseController::listDevices(const QUuid &deviceId)
{
	auto db = _threadStore.localData().database();

	Query loadDevicesQuery(db);
	loadDevicesQuery.prepare(QStringLiteral("SELECT devices.id, name, fingerprint "
											"FROM devices "
											"INNER JOIN users ON devices.userid = users.id "
											"WHERE devices.id != ? "
											"AND devices.userid = ( "
											"	SELECT userid FROM devices "
											"	WHERE id = ? "
											")"));
	loadDevicesQuery.addBindValue(deviceId);
	loadDevicesQuery.addBindValue(deviceId);
	loadDevicesQuery.exec();

	QList<std::tuple<QUuid, QString, QByteArray>> resList;
	while(loadDevicesQuery.next()) {
		resList.append(std::tuple<QUuid, QString, QByteArray> {
						   loadDevicesQuery.value(0).toUuid(),
						   loadDevicesQuery.value(1).toString(),
						   loadDevicesQuery.value(2).toByteArray()
					   });
	}
	return resList;
}

void DatabaseController::removeDevice(const QUuid &deviceId, const QUuid &deleteId)
{
	auto db = _threadStore.localData().database();
	if(!db.transaction())
		throw DatabaseException(db);

	try {
		Query userIdQuery(db);
		userIdQuery.prepare(QStringLiteral("SELECT userid FROM devices WHERE id = ?"));
		userIdQuery.addBindValue(deviceId);
		userIdQuery.exec();
		if(!userIdQuery.first()) {
			if(!db.commit())
				throw DatabaseException(db);
			return;
		}

		auto userId = userIdQuery.value(0).toULongLong();
		Query deleteDeviceQuery(db);
		deleteDeviceQuery.prepare(QStringLiteral("DELETE FROM devices "
												 "WHERE id = ? AND userid = ?"));
		deleteDeviceQuery.addBindValue(deleteId);
		deleteDeviceQuery.addBindValue(userId);
		deleteDeviceQuery.exec();

		Query deleteUserQuery(db);
		deleteUserQuery.prepare(QStringLiteral("DELETE FROM users WHERE id = ? "
											   "AND NOT EXISTS ( "
											   "	SELECT 1 FROM devices "
											   "	WHERE userid = ? "
											   ")"));
		deleteUserQuery.addBindValue(userId);
		deleteUserQuery.addBindValue(userId);
		deleteUserQuery.exec();

		if(!db.commit())
			throw DatabaseException(db);
	} catch(...) {
		db.rollback();
		throw;
	}
}

void DatabaseController::addChange(const QUuid &deviceId, const QByteArray &dataId, const quint32 keyIndex, const QByteArray &salt, const QByteArray &data)
{
	auto db = _threadStore.localData().database();
	if(!db.transaction())
		throw DatabaseException(db);

	try {
		// delete the entry, in case it already exists. Will do nothing if nothing exists
		Query deleteOldQuery(db);
		deleteOldQuery.prepare(QStringLiteral("DELETE FROM datachanges WHERE deviceid = ? AND dataid = ?"));
		deleteOldQuery.addBindValue(deviceId);
		deleteOldQuery.addBindValue(dataId);
		deleteOldQuery.exec();

		// add the data change
		Query addChangeQuery(db);
		addChangeQuery.prepare(QStringLiteral("INSERT INTO datachanges (deviceid, dataid, keyid, salt, data) VALUES(?, ?, ?, ?, ?)"));
		addChangeQuery.addBindValue(deviceId);
		addChangeQuery.addBindValue(dataId);
		addChangeQuery.addBindValue(keyIndex);
		addChangeQuery.addBindValue(salt);
		addChangeQuery.addBindValue(data);
		addChangeQuery.exec();
		auto nId = addChangeQuery.lastInsertId();
		if(!nId.isValid()){
			db.rollback();
			throw DatabaseException(QSqlError(QString(), QStringLiteral("Unable to get id of last inserted data change")));
		}

		// update device changes
		Query updateDevicesQuery(db);
		updateDevicesQuery.prepare(QStringLiteral("INSERT INTO devicechanges(dataid, deviceid) "
												  "SELECT ? AS dataid, devices.id AS deviceid FROM devices "
												  "INNER JOIN users ON devices.userid = users.id "
												  "WHERE devices.id != ? "
												  "AND devices.userid = ( "
												  "	SELECT userid FROM devices "
												  "	WHERE id = ? "
												  ")"));
		updateDevicesQuery.addBindValue(nId);
		updateDevicesQuery.addBindValue(deviceId);
		updateDevicesQuery.addBindValue(deviceId);
		updateDevicesQuery.exec();
		auto affected = updateDevicesQuery.numRowsAffected();

		if(affected == 0) { //no devices to be notified -> remove the data again
			Query removeChangeQuery(db);
			removeChangeQuery.prepare(QStringLiteral("DELETE FROM datachanges WHERE id = ?"));
			removeChangeQuery.addBindValue(nId);
			removeChangeQuery.exec();
		}

		if(!db.commit())
			throw DatabaseException(db);
	} catch(...) {
		db.rollback();
		throw;
	}
}

quint32 DatabaseController::changeCount(const QUuid &deviceId)
{
	auto db = _threadStore.localData().database();
	Query countChangesQuery(db);
	countChangesQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM devicechanges WHERE deviceid = ?"));
	countChangesQuery.addBindValue(deviceId);
	countChangesQuery.exec();
	if(countChangesQuery.first())
		return countChangesQuery.value(0).toUInt();
	else
		return 0;
}

QList<std::tuple<quint64, quint32, QByteArray, QByteArray>> DatabaseController::loadNextChanges(const QUuid &deviceId, quint32 count, quint32 skip)
{
	auto db = _threadStore.localData().database();

	Query loadChangesQuery(db);
	loadChangesQuery.prepare(QStringLiteral("SELECT id, keyid, salt, data FROM datachanges "
											"INNER JOIN devicechanges ON datachanges.id = devicechanges.dataid "
											"WHERE devicechanges.deviceid = ? "
											"ORDER BY datachanges.id "
											"LIMIT ? OFFSET ?"));
	loadChangesQuery.addBindValue(deviceId);
	loadChangesQuery.addBindValue(count);
	loadChangesQuery.addBindValue(skip);
	loadChangesQuery.exec();

	QList<std::tuple<quint64, quint32, QByteArray, QByteArray>> resList;
	while(loadChangesQuery.next()) {
		resList.append(std::tuple<quint64, quint32, QByteArray, QByteArray> {
						   loadChangesQuery.value(0).toULongLong(),
						   loadChangesQuery.value(1).toUInt(),
						   loadChangesQuery.value(2).toByteArray(),
						   loadChangesQuery.value(3).toByteArray()
					   });
	}
	return resList;
}

void DatabaseController::completeChange(const QUuid &deviceId, quint64 dataIndex)
{
	auto db = _threadStore.localData().database();
	if(!db.transaction())
		throw DatabaseException(db);

	try {
		Query deleteChangeQuery(db);
		deleteChangeQuery.prepare(QStringLiteral("DELETE FROM devicechanges WHERE deviceid = ? AND dataid = ?"));
		deleteChangeQuery.addBindValue(deviceId);
		deleteChangeQuery.addBindValue(dataIndex);
		deleteChangeQuery.exec();

		Query deleteDataQuery(db);
		deleteDataQuery.prepare(QStringLiteral("DELETE FROM datachanges WHERE id = ? "
											   "AND NOT EXISTS ( "
											   "	SELECT 1 FROM devicechanges "
											   "	WHERE dataid = ? "
											   ")"));
		deleteDataQuery.addBindValue(dataIndex);
		deleteDataQuery.addBindValue(dataIndex);
		deleteDataQuery.exec();

		if(!db.commit())
			throw DatabaseException(db);
	} catch(...) {
		db.rollback();
		throw;
	}
}

void DatabaseController::onNotify(const QString &name, QSqlDriver::NotificationSource source, const QVariant &payload)
{
	Q_UNUSED(source)
	if(name == QStringLiteral("deviceDataEvent")) {
		auto device = payload.toUuid();
		if(device.isNull())
			qWarning() << "Invalid event data for deviceDataEvent:" << payload;
		else
			emit notifyChanged(device);
	}
}

void DatabaseController::timeout()
{
	auto db = _threadStore.localData().database();
	QSqlQuery query(db);
	if(!query.exec(QStringLiteral("SELECT NULL"))) {
		qCritical().noquote() << "Keepalive query failed! Server might needs to be restarted in order to make live updates work again."
								 "\nDatabase Error:"
							  << query.lastError().text();
	} else
		qDebug() << "Keepalive succeeded";
}

void DatabaseController::initDatabase()
{
	auto db = _threadStore.localData().database();
	if(!db.isOpen()) {
		emit databaseInitDone(false);
		return;
	}

	try {
//#define AUTO_DROP_TABLES
#ifdef AUTO_DROP_TABLES
		QSqlQuery dropQuery(db);
		if(!dropQuery.exec(QStringLiteral("DROP TABLE IF EXISTS devicechanges, datachanges, devices, users CASCADE"))) {
			qWarning() << "Failed to drop tables with error:"
					   << qPrintable(dropQuery.lastError().text());
		} else
			qInfo() << "Dropped all existing tables";
#endif

		static const auto features = {
			QSqlDriver::Transactions,
			QSqlDriver::BLOB,
			QSqlDriver::PreparedQueries,
			QSqlDriver::PositionalPlaceholders,
			QSqlDriver::LastInsertId,
			QSqlDriver::EventNotifications
		};
		auto driver = db.driver();
		foreach(auto feature, features) {
			if(!driver->hasFeature(feature))
				throw DatabaseException(QSqlError(QStringLiteral("Driver does not support feature %1").arg(feature)));
		}

		if(!db.tables().contains(QStringLiteral("users"))) {
			QSqlQuery createUsers(db);
			if(!createUsers.exec(QStringLiteral("CREATE TABLE users ( "
											   "	id			BIGSERIAL PRIMARY KEY NOT NULL, "
											   "	keycount	INT NOT NULL DEFAULT 0, "
											   "	quota		BIGINT NOT NULL DEFAULT 0, "
											   "	CHECK(quota < 10000000) " //10 MB
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
												  "		fingerprint	BYTEA NOT NULL, "
												  "		lastlogin	DATE NOT NULL DEFAULT 'today' "
												  ")"))) {
				throw DatabaseException(createDevices);
			}
		}

		if(!db.tables().contains(QStringLiteral("datachanges"))) {
			QSqlQuery createDataChanges(db);
			if(!createDataChanges.exec(QStringLiteral("CREATE TABLE datachanges ( "
													  "		id			BIGSERIAL PRIMARY KEY NOT NULL, "
													  "		deviceid	UUID NOT NULL REFERENCES devices(id) ON DELETE CASCADE, "
													  "		dataid		BYTEA NOT NULL, "
													  "		keyid		INT NOT NULL, "
													  "		salt		BYTEA NOT NULL, "
													  "		data		BYTEA NOT NULL, "
													  "		UNIQUE(deviceid, dataid) "
													  ")"))) {
				throw DatabaseException(createDataChanges);
			}

			QSqlQuery createUpquotaFn(db);
			if(!createUpquotaFn.exec(QStringLiteral("CREATE OR REPLACE FUNCTION upquota() RETURNS TRIGGER AS "
													"$BODY$ "
													"BEGIN "
													"	UPDATE users SET quota = quota + octet_length(NEW.data) WHERE id = ( "
													"		SELECT userid FROM devices WHERE id = NEW.deviceid "
													"	); "
													"	RETURN NEW; "
													"END; "
													"$BODY$ "
													"LANGUAGE plpgsql;"))) {
				throw DatabaseException(createUpquotaFn);
			}

			QSqlQuery createUpquotaTrigger(db);
			if(!createUpquotaTrigger.exec(QStringLiteral("CREATE TRIGGER add_data_trigger "
														 "AFTER INSERT "
														 "ON datachanges "
														 "FOR EACH ROW "
														 "EXECUTE PROCEDURE upquota();"))) {
				throw DatabaseException(createUpquotaTrigger);
			}

			QSqlQuery createDownquotaFn(db);
			if(!createDownquotaFn.exec(QStringLiteral("CREATE OR REPLACE FUNCTION downquota() RETURNS TRIGGER AS "
													  "$BODY$ "
													  "BEGIN "
													  "		UPDATE users SET quota = GREATEST(quota - octet_length(OLD.data), 0) WHERE id = ( "
													  "			SELECT userid FROM devices WHERE id = OLD.deviceid "
													  "		); "
													  "		RETURN OLD; "
													  "END; "
													  "$BODY$ "
													  "LANGUAGE plpgsql;"))) {
				throw DatabaseException(createDownquotaFn);
			}

			QSqlQuery createDownquotaTrigger(db);
			if(!createDownquotaTrigger.exec(QStringLiteral("CREATE TRIGGER remove_data_trigger "
														   "AFTER DELETE "
														   "ON datachanges "
														   "FOR EACH ROW "
														   "EXECUTE PROCEDURE downquota();"))) {
				throw DatabaseException(createDownquotaTrigger);
			}
		}

		if(!db.tables().contains(QStringLiteral("devicechanges"))) {
			QSqlQuery createDeviceChanges(db);
			if(!createDeviceChanges.exec(QStringLiteral("CREATE TABLE devicechanges ( "
														"	deviceid	UUID NOT NULL REFERENCES devices(id) ON DELETE CASCADE, "
														"	dataid		BIGINT NOT NULL REFERENCES datachanges(id) ON DELETE CASCADE, "
														"	PRIMARY KEY(deviceid, dataid) "
														")"))) {
				throw DatabaseException(createDeviceChanges);
			}

			QSqlQuery createNotifyFn(db);
			if(!createNotifyFn.exec(QStringLiteral("CREATE OR REPLACE FUNCTION notifyDeviceChange() RETURNS TRIGGER AS "
												   "$BODY$ "
												   "BEGIN "
												   "	PERFORM pg_notify('deviceDataEvent', NEW.deviceid::text); "
												   "	RETURN NEW; "
												   "END; "
												   "$BODY$ "
												   "LANGUAGE plpgsql VOLATILE;"))) {
				throw DatabaseException(createNotifyFn);
			}

			QSqlQuery createNotifyTrigger(db);
			if(!createNotifyTrigger.exec(QStringLiteral("CREATE TRIGGER device_change_trigger "
														"AFTER INSERT "
														"ON devicechanges "
														"FOR EACH ROW "
														"EXECUTE PROCEDURE notifyDeviceChange();"))) {
				throw DatabaseException(createNotifyTrigger);
			}
		}

		emit databaseInitDone(true);
	} catch(DatabaseException &e) {
		qCritical() << "Failed to setup database:" << e.what();
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
	_msg("\n ==> Error: " + error.text().toUtf8())
{}

DatabaseException::DatabaseException(QSqlDatabase db) :
	DatabaseException(db.lastError())
{}

DatabaseException::DatabaseException(const QSqlQuery &query) :
	_error(query.lastError()),
	_msg("\n ==> Query: " + query.executedQuery().toUtf8() +
		 "\n ==> Error: " + query.lastError().text().toUtf8())
{}

QSqlError DatabaseException::error() const
{
	return _error;
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
		throw DatabaseException(*this);
}

void Query::exec()
{
	if(!QSqlQuery::exec())
		throw DatabaseException(*this);
}
