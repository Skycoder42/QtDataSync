#include "databasecontroller.h"
#include "app.h"

#include <QtCore/QJsonDocument>

#include <QtSql/QSqlQuery>

#include <QtConcurrent/QtConcurrentRun>

#if QT_HAS_INCLUDE(<chrono>)
#define scdtime(x) x
#else
#define scdtime(x) duration_cast<milliseconds>(x).count()
#endif

using namespace QtDataSync;
using namespace std::chrono;
using std::tuple;
using std::make_tuple;
using std::get;

namespace {

class Query : public QSqlQuery
{
public:
	explicit Query(QSqlDatabase db);

	void prepare(const QString &query);
	void exec();
};

}

QThreadStorage<DatabaseController::DatabaseWrapper> DatabaseController::_threadStore;

DatabaseController::DatabaseController(QObject *parent) :
	QObject(parent),
	_keepAliveTimer(nullptr),
	_cleanupTimer(nullptr)
{}

void DatabaseController::initialize()
{
	auto quota = qApp->configuration()->value(QStringLiteral("quota/limit"), 10485760).toULongLong(); //10MB
	auto force = qApp->configuration()->value(QStringLiteral("quota/force"), false).toBool();
	QtConcurrent::run(qApp->threadPool(), this, &DatabaseController::initDatabase,
					  quota, force);
}

void DatabaseController::cleanupDevices()
{
	auto offlineSinceDays = qApp->configuration()->value(QStringLiteral("cleanup/interval"),
														 90ull) //default interval of ca 3 months
							.toULongLong();
	if(offlineSinceDays == 0)
		return;

	QtConcurrent::run(qApp->threadPool(), [this, offlineSinceDays]() {
		try {
			auto db = _threadStore.localData().database();
			if(!db.transaction())
				throw DatabaseException(db);

			try {
				Query deleteDevicesQuery(db);
				deleteDevicesQuery.prepare(QStringLiteral("DELETE FROM devices "
														  "WHERE (current_date - lastlogin) > ?"));
				deleteDevicesQuery.addBindValue(offlineSinceDays);
				deleteDevicesQuery.exec();
				auto devNum = deleteDevicesQuery.numRowsAffected();

				Query deleteUsersQuery(db);
				deleteUsersQuery.prepare(QStringLiteral("DELETE FROM users "
														"WHERE NOT EXISTS ( "
														"	SELECT 1 FROM devices "
														"	WHERE userid = users.id "
														")"));
				deleteUsersQuery.exec();
				auto usrNum = deleteUsersQuery.numRowsAffected();

				if(!db.commit())
					throw DatabaseException(db);

				if(devNum == 0 && usrNum == 0)
					qDebug() << "Successfully cleanup up database. No devices or users removed";
				else {
					qInfo() << "Successfully cleanup up database. Removed" << devNum
							<< "devices and" << usrNum << "users";
				}
			} catch(...) {
				db.rollback();
				throw;
			}
		} catch (DatabaseException &e) {
			qWarning() << "Database cleanup failed with error:" << e.what();
		}
	});
}

QUuid DatabaseController::addNewDevice(const QString &name, const QByteArray &signScheme, const QByteArray &signKey, const QByteArray &cryptScheme, const QByteArray &cryptKey, const QByteArray &fingerprint, const QByteArray &keyCmac)
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
												 "(id, userid, name, signscheme, signkey, cryptscheme, cryptkey, fingerprint, keymac) "
												 "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)"));
		createDeviceQuery.addBindValue(deviceId);
		createDeviceQuery.addBindValue(userId);
		createDeviceQuery.addBindValue(name);
		createDeviceQuery.addBindValue(QString::fromUtf8(signScheme));
		createDeviceQuery.addBindValue(signKey);
		createDeviceQuery.addBindValue(QString::fromUtf8(cryptScheme));
		createDeviceQuery.addBindValue(cryptKey);
		createDeviceQuery.addBindValue(fingerprint);
		createDeviceQuery.addBindValue(keyCmac);
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
											 "VALUES(?, deviceUserId(?), ?, ?, ?, ?, ?, ?) "));
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
	updateNameQuery.prepare(QStringLiteral("UPDATE devices SET name = ?, lastlogin = current_date "
										   "WHERE id = ?"));
	updateNameQuery.addBindValue(name);
	updateNameQuery.addBindValue(deviceId);
	updateNameQuery.exec();
}

bool DatabaseController::updateCmac(const QUuid &deviceId, quint32 keyIndex, const QByteArray &cmac)
{
	auto db = _threadStore.localData().database();
	if(!db.transaction())
		throw DatabaseException(db);

	try {
		Query updateCmacQuery(db);
		updateCmacQuery.prepare(QStringLiteral("UPDATE devices SET keymac = ? "
											   "WHERE id = ? AND ( "
											   "	SELECT keycount FROM users "
											   "	WHERE id = deviceUserId(?) "
											   ") = ?"));
		updateCmacQuery.addBindValue(cmac);
		updateCmacQuery.addBindValue(deviceId);
		updateCmacQuery.addBindValue(deviceId);
		updateCmacQuery.addBindValue(keyIndex);
		updateCmacQuery.exec();

		if(updateCmacQuery.numRowsAffected() > 0) {
			Query removeChangesQuery(db);
			removeChangesQuery.prepare(QStringLiteral("DELETE FROM keychanges "
													  "WHERE deviceid = ? "
													  "AND keyindex = ?"));
			removeChangesQuery.addBindValue(deviceId);
			removeChangesQuery.addBindValue(keyIndex);
			removeChangesQuery.exec();

			if(!db.commit())
				throw DatabaseException(db);
			return true;
		} else {
			db.rollback();
			return false;
		}
	} catch(...) {
		db.rollback();
		throw;
	}
}

QList<tuple<QUuid, QString, QByteArray>> DatabaseController::listDevices(const QUuid &deviceId)
{
	auto db = _threadStore.localData().database();
	Query loadDevicesQuery(db);
	loadDevicesQuery.prepare(QStringLiteral("SELECT devices.id, name, fingerprint "
											"FROM devices "
											"INNER JOIN users ON devices.userid = users.id "
											"WHERE devices.id != ? "
											"AND devices.userid = deviceUserId(?)"));
	loadDevicesQuery.addBindValue(deviceId);
	loadDevicesQuery.addBindValue(deviceId);
	loadDevicesQuery.exec();

	QList<tuple<QUuid, QString, QByteArray>> resList;
	while(loadDevicesQuery.next()) {
		resList.append(make_tuple(
						   loadDevicesQuery.value(0).toUuid(),
						   loadDevicesQuery.value(1).toString(),
						   loadDevicesQuery.value(2).toByteArray()
					   ));
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
		userIdQuery.prepare(QStringLiteral("SELECT deviceUserId(?)"));
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

bool DatabaseController::addChange(const QUuid &deviceId, const QByteArray &dataId, const quint32 keyIndex, const QByteArray &salt, const QByteArray &data)
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
		addChangeQuery.prepare(QStringLiteral("INSERT INTO datachanges (deviceid, dataid, keyid, salt, data) "
											  "VALUES(?, ?, ?, ?, ?)"));
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
												  "AND devices.userid = deviceUserId(?)"));
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
		return true;
	} catch(DatabaseException &e) {
		//check_violation from https://www.postgresql.org/docs/current/static/errcodes-appendix.html
		auto isCheck = (e.error().nativeErrorCode() == QStringLiteral("23514"));
		db.rollback();
		if(isCheck) {
			qWarning() << "Device" << deviceId << "hit quota limit";
			return false;
		} else
			throw;
	} catch(...) {
		db.rollback();
		throw;
	}
}

bool DatabaseController::addDeviceChange(const QUuid &deviceId, const QUuid &targetId, const QByteArray &dataId, const quint32 keyIndex, const QByteArray &salt, const QByteArray &data)
{
	auto db = _threadStore.localData().database();
	if(!db.transaction())
		throw DatabaseException(db);

	try {
		// add the data change (or ignore, if already existing)
		Query addChangeQuery(db);
		addChangeQuery.prepare(QStringLiteral("INSERT INTO datachanges (deviceid, dataid, keyid, salt, data) "
											  "VALUES(?, ?, ?, ?, ?) "
											  "ON CONFLICT(deviceid, dataid) DO NOTHING "
											  "RETURNING id"));
		addChangeQuery.addBindValue(deviceId);
		addChangeQuery.addBindValue(dataId);
		addChangeQuery.addBindValue(keyIndex);
		addChangeQuery.addBindValue(salt);
		addChangeQuery.addBindValue(data);
		addChangeQuery.exec();

		//get the id of the data
		QVariant nId;
		if(addChangeQuery.first())
			nId = addChangeQuery.value(0);
		else {//insert was ignored, as data already exists
			Query getIdQuery(db);
			getIdQuery.prepare(QStringLiteral("SELECT id FROM datachanges WHERE deviceid = ? AND dataid = ?"));
			getIdQuery.addBindValue(deviceId);
			getIdQuery.addBindValue(dataId);
			getIdQuery.exec();
			if(!getIdQuery.first()){
				db.rollback();
				throw DatabaseException(QSqlError(QString(), QStringLiteral("Unable to get id of last inserted data change")));
			} else
				nId = getIdQuery.value(0);
		}

		// add a change for the device (or ignore)
		Query updateDevicesQuery(db);
		updateDevicesQuery.prepare(QStringLiteral("INSERT INTO devicechanges(dataid, deviceid) "
												  "VALUES(?, ?) "
												  "ON CONFLICT DO NOTHING"));
		updateDevicesQuery.addBindValue(nId);
		updateDevicesQuery.addBindValue(targetId);
		updateDevicesQuery.exec();

		if(!db.commit())
			throw DatabaseException(db);
		return true;
	} catch(DatabaseException &e) {
		//check_violation from https://www.postgresql.org/docs/current/static/errcodes-appendix.html
		auto isCheck = (e.error().nativeErrorCode() == QStringLiteral("23514"));
		db.rollback();
		if(isCheck) {
			qWarning() << "Device" << deviceId << "hit quota limit";
			return false;
		} else
			throw;
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

QList<tuple<quint64, quint32, QByteArray, QByteArray>> DatabaseController::loadNextChanges(const QUuid &deviceId, quint32 count, quint32 skip)
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

	QList<tuple<quint64, quint32, QByteArray, QByteArray>> resList;
	while(loadChangesQuery.next()) {
		resList.append(make_tuple(
						   (quint64)loadChangesQuery.value(0).toULongLong(),
						   (quint32)loadChangesQuery.value(1).toUInt(),
						   loadChangesQuery.value(2).toByteArray(),
						   loadChangesQuery.value(3).toByteArray()
					   ));
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

QList<tuple<QUuid, QByteArray, QByteArray, QByteArray>> DatabaseController::tryKeyChange(const QUuid &deviceId, quint32 proposedIndex, int &offset)
{
	offset = -1;

	auto db = _threadStore.localData().database();
	if(!db.transaction())
		throw DatabaseException(db);

	try {
		//load current key index
		Query readIndexQuery(db);
		readIndexQuery.prepare(QStringLiteral("SELECT keycount, id FROM users "
											  "WHERE id = deviceUserId(?)"));
		readIndexQuery.addBindValue(deviceId);
		readIndexQuery.exec();
		if(!readIndexQuery.first())
			throw DatabaseException(db);
		auto currentIndex = readIndexQuery.value(0).toUInt();
		auto userId = readIndexQuery.value(1).toULongLong();
		offset = proposedIndex - currentIndex;
		if(offset != 1) { //only when 1 the rest is needed
			if(!db.commit())
				throw DatabaseException(db);
			return {};
		}

		//check if any device still has keychanges
		Query hasKeyChangesQuery(db);
		hasKeyChangesQuery.prepare(QStringLiteral("SELECT 1 FROM keychanges "
												  "INNER JOIN devices ON keychanges.deviceid = devices.id "
												  "INNER JOIN users ON devices.userid = users.id "
												  "WHERE users.id = ?"));
		hasKeyChangesQuery.addBindValue(userId);
		hasKeyChangesQuery.exec();
		if(hasKeyChangesQuery.first()) {
			offset = -1;
			if(!db.commit())
				throw DatabaseException(db);
			return {};
		}

		//load device keys
		Query deviceKeysQuery(db);
		deviceKeysQuery.prepare(QStringLiteral("SELECT id, cryptscheme, cryptkey, keymac FROM devices "
											   "WHERE id != ? "
											   "AND userid = ?"));
		deviceKeysQuery.addBindValue(deviceId);
		deviceKeysQuery.addBindValue(userId);
		deviceKeysQuery.exec();

		QList<tuple<QUuid, QByteArray, QByteArray, QByteArray>> result;
		while(deviceKeysQuery.next()) {
			result.append(make_tuple(
							  deviceKeysQuery.value(0).toUuid(),
							  deviceKeysQuery.value(1).toByteArray(),
							  deviceKeysQuery.value(2).toByteArray(),
							  deviceKeysQuery.value(3).toByteArray()
						  ));
		}

		if(!db.commit())
			throw DatabaseException(db);
		return result;
	} catch(...) {
		offset = -1;
		db.rollback();
		throw;
	}
}

bool DatabaseController::updateExchangeKey(const QUuid &deviceId, quint32 keyIndex, const QByteArray &scheme, const QByteArray &cmac, const QList<tuple<QUuid, QByteArray, QByteArray>> &deviceKeys)
{
	auto db = _threadStore.localData().database();
	if(!db.transaction())
		throw DatabaseException(db);

	try {
		Query updateKeyCountQuery(db);
		updateKeyCountQuery.prepare(QStringLiteral("UPDATE users SET keycount = keycount + 1"
												   "WHERE id = deviceUserId(?) "
												   "AND (keycount + 1) = ? "
												   "RETURNING id"));
		updateKeyCountQuery.addBindValue(deviceId);
		updateKeyCountQuery.addBindValue(keyIndex);
		updateKeyCountQuery.exec();
		if(updateKeyCountQuery.numRowsAffected() != 1) {
			db.rollback();
			return false;
		}
		if(!updateKeyCountQuery.first())
			throw DatabaseException(db);
		auto userId = updateKeyCountQuery.value(0).toULongLong();

		for(auto device : deviceKeys) {
			//check if the device belongs to the same user
			Query checkAllowedQuery(db);
			checkAllowedQuery.prepare(QStringLiteral("SELECT 1 FROM devices "
													 "WHERE id = ? "
													 "AND userid = ?"));
			checkAllowedQuery.addBindValue(get<0>(device));
			checkAllowedQuery.addBindValue(userId);
			checkAllowedQuery.exec();
			if(!checkAllowedQuery.first())
				throw DatabaseException(db);

			//add the keychange
			Query addKeyQuery(db);
			addKeyQuery.prepare(QStringLiteral("INSERT INTO keychanges "
											   "(deviceid, keyindex, scheme, key, verifymac) "
											   "VALUES(?, ?, ?, ?, ?)"));
			addKeyQuery.addBindValue(get<0>(device));
			addKeyQuery.addBindValue(keyIndex);
			addKeyQuery.addBindValue(QString::fromUtf8(scheme));
			addKeyQuery.addBindValue(get<1>(device));
			addKeyQuery.addBindValue(get<2>(device));
			addKeyQuery.exec();
		}

		//update the cmac
		Query updateCmacQuery(db);
		updateCmacQuery.prepare(QStringLiteral("UPDATE devices SET keymac = ? "
											   "WHERE id = ?"));
		updateCmacQuery.addBindValue(cmac);
		updateCmacQuery.addBindValue(deviceId);
		updateCmacQuery.exec();

		if(!db.commit())
			throw DatabaseException(db);
		return true;
	} catch(...) {
		db.rollback();
		throw;
	}
}

tuple<quint32, QByteArray, QByteArray, QByteArray> DatabaseController::loadKeyChanges(const QUuid &deviceId)
{
	auto db = _threadStore.localData().database();

	Query keyChangesQuery(db);
	keyChangesQuery.prepare(QStringLiteral("SELECT keyindex, scheme, key, verifymac FROM keychanges "
										   "WHERE deviceid = ? "
										   "ORDER BY keyindex ASC"));
	keyChangesQuery.addBindValue(deviceId);
	keyChangesQuery.exec();

	QList<tuple<quint32, QByteArray, QByteArray, QByteArray>> result;
	if(keyChangesQuery.first()) {
		return make_tuple(
			(quint32)keyChangesQuery.value(0).toUInt(),
			keyChangesQuery.value(1).toByteArray(),
			keyChangesQuery.value(2).toByteArray(),
			keyChangesQuery.value(3).toByteArray()
		);
	} else
		return make_tuple((quint32)0, QByteArray(), QByteArray(), QByteArray());
}

void DatabaseController::dbInitDone(bool success)
{
	if(success) { //done on the main thread to make sure the connection does not die with threads
		auto liveSync = qApp->configuration()->value(QStringLiteral("livesync"), true).toBool();
		if(liveSync) {
			auto driver = _threadStore.localData().database().driver();
			connect(driver, QOverload<const QString &, QSqlDriver::NotificationSource, const QVariant &>::of(&QSqlDriver::notification),
					this, &DatabaseController::onNotify);
			if(!driver->subscribeToNotification(QStringLiteral("deviceDataEvent"))) {
				qCritical() << "Unabled to notify to change events. Devices will not receive updates!";
				success = false;
			} else
				qInfo() << "Live sync enabled";
		} else
			qInfo() << "Live sync disabled";
	}

	if(success) {
		auto delay = qApp->configuration()->value(QStringLiteral("database/keepaliveInterval"), 5).toInt();
		if(delay > 0) {
			_keepAliveTimer = new QTimer(this);
			_keepAliveTimer->setInterval(scdtime(minutes(delay)));
			_keepAliveTimer->setTimerType(Qt::VeryCoarseTimer);
			connect(_keepAliveTimer, &QTimer::timeout,
					this, &DatabaseController::timeout);
			_keepAliveTimer->start();
			qInfo() << "Keepalives enabled";
		} else
			qInfo() << "Keepalives disabled";
	}

	if(success) {
		if(qApp->configuration()->value(QStringLiteral("cleanup/auto"), true).toBool()) {
			_cleanupTimer = new QTimer(this);
			_cleanupTimer->setInterval(scdtime(hours(24)));
			_cleanupTimer->setTimerType(Qt::VeryCoarseTimer);
			connect(_cleanupTimer, &QTimer::timeout,
					this, &DatabaseController::cleanupDevices);
			_cleanupTimer->start();

			auto offlineSinceDays = qApp->configuration()->value(QStringLiteral("cleanup/interval"),
																 90ull) //default interval of ca 3 months
									.toULongLong();
			qInfo() << "Automatic cleanup enabled with" << offlineSinceDays << "day intervals";
		} else
			qInfo() << "Automatic cleanup disabled";
	}

	emit databaseInitDone(success);
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

void DatabaseController::initDatabase(quint64 quota, bool forceQuota)
{
	auto db = _threadStore.localData().database();
	if(!db.isOpen()) {
		QMetaObject::invokeMethod(this, "dbInitDone", Qt::QueuedConnection,
								  Q_ARG(bool, false));
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
		for(auto feature : features) {
			if(!driver->hasFeature(feature))
				throw DatabaseException(QSqlError(QStringLiteral("Driver does not support feature %1").arg(feature)));
		}

		if(!db.tables().contains(QStringLiteral("users"))) {
			QSqlQuery createUsers(db);
			if(!createUsers.exec(QStringLiteral("CREATE TABLE users ( "
											   "	id			BIGSERIAL PRIMARY KEY NOT NULL, "
											   "	keycount	INT NOT NULL DEFAULT 0, "
											   "	quota		BIGINT NOT NULL DEFAULT 0, "
											   "	quotalimit	BIGINT NOT NULL DEFAULT %1, "
											   "	CHECK(quota < quotalimit) " //10 MB
											   ")")
								 .arg(quota))) {
				throw DatabaseException(createUsers);
			}

			qDebug() << "Created table users (+ functions and triggers)";
		} else
			updateQuotaLimit(quota, forceQuota);

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
												  "		keymac		BYTEA, "
												  "		lastlogin	DATE NOT NULL DEFAULT current_date "
												  ")"))) {
				throw DatabaseException(createDevices);
			}

			QSqlQuery createUserIdFn(db);
			if(!createUserIdFn.exec(QStringLiteral("CREATE OR REPLACE FUNCTION deviceUserId(device UUID) "
												   "RETURNS BIGINT AS $BODY$ "
												   "DECLARE "
												   "	uid BIGINT; "
												   "BEGIN "
												   "	SELECT devices.userid INTO uid FROM devices WHERE id = device; "
												   "	RETURN uid; "
												   "END; "
												   "$BODY$ LANGUAGE plpgsql;"))) {
				throw DatabaseException(createUserIdFn);
			}

			qDebug() << "Created table devices (+ functions and triggers)";
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
			if(!createUpquotaFn.exec(QStringLiteral("CREATE OR REPLACE FUNCTION upquota() "
													"RETURNS TRIGGER AS $BODY$ "
													"BEGIN "
													"	UPDATE users SET quota = quota + octet_length(NEW.data) "
													"	WHERE id = deviceUserId(NEW.deviceid); "
													"	RETURN NEW; "
													"END; "
													"$BODY$ LANGUAGE plpgsql;"))) {
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
			if(!createDownquotaFn.exec(QStringLiteral("CREATE OR REPLACE FUNCTION downquota() "
													  "RETURNS TRIGGER AS $BODY$ "
													  "BEGIN "
													  "		UPDATE users SET quota = GREATEST(quota - octet_length(OLD.data), 0) "
													  "		WHERE id = deviceUserId(OLD.deviceid); "
													  "		RETURN OLD; "
													  "END; "
													  "$BODY$ LANGUAGE plpgsql;"))) {
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

			qDebug() << "Created table datachanges (+ functions and triggers)";
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
			if(!createNotifyFn.exec(QStringLiteral("CREATE OR REPLACE FUNCTION notifyDeviceChange() RETURNS TRIGGER AS $BODY$ "
												   "BEGIN "
												   "	PERFORM pg_notify('deviceDataEvent', NEW.deviceid::text); "
												   "	RETURN NEW; "
												   "END; "
												   "$BODY$ LANGUAGE plpgsql VOLATILE;"))) {
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

			qDebug() << "Created table devicechanges (+ functions and triggers)";
		}

		if(!db.tables().contains(QStringLiteral("keychanges"))) {
			QSqlQuery createKeyChanges(db);
			if(!createKeyChanges.exec(QStringLiteral("CREATE TABLE keychanges ( "
														"	deviceid	UUID PRIMARY KEY NOT NULL REFERENCES devices(id) ON DELETE CASCADE, "
														"	keyindex	INT NOT NULL, "
														"	scheme		TEXT NOT NULL, "
														"	key			BYTEA NOT NULL, "
														"	verifymac	BYTEA NOT NULL "
														")"))) {
				throw DatabaseException(createKeyChanges);
			}

			qDebug() << "Created table keychanges (+ functions and triggers)";
		}

		QMetaObject::invokeMethod(this, "dbInitDone", Qt::QueuedConnection,
								  Q_ARG(bool, true));
	} catch(DatabaseException &e) {
		qCritical() << "Failed to setup database:" << e.what();
		QMetaObject::invokeMethod(this, "dbInitDone", Qt::QueuedConnection,
								  Q_ARG(bool, false));
	}
}

void DatabaseController::updateQuotaLimit(quint64 quota, bool forceQuota)
{
	auto db = _threadStore.localData().database();
	if(!db.transaction())
		throw DatabaseException(db);

	try {
		if(forceQuota) {
			Query deleteOverQuotaDevicesQuery(db);
			deleteOverQuotaDevicesQuery.prepare(QStringLiteral("DELETE FROM devices "
															   "WHERE userid IN ( "
															   "	SELECT id FROM users "
															   "	WHERE quotalimit != ? "
															   "	AND quota >= ? "
															   ")"));
			deleteOverQuotaDevicesQuery.addBindValue(quota);
			deleteOverQuotaDevicesQuery.addBindValue(quota);
			deleteOverQuotaDevicesQuery.exec();
			auto devNum = deleteOverQuotaDevicesQuery.numRowsAffected();

			Query deleteOverQuotaUsersQuery(db);
			deleteOverQuotaUsersQuery.prepare(QStringLiteral("DELETE FROM users "
															 "WHERE quotalimit != ? "
															 "AND quota >= ?"));
			deleteOverQuotaUsersQuery.addBindValue(quota);
			deleteOverQuotaUsersQuery.addBindValue(quota);
			deleteOverQuotaUsersQuery.exec();
			auto usrNum = deleteOverQuotaUsersQuery.numRowsAffected();

			if(usrNum == 0 && devNum == 0)
				qDebug() << "No users or devices deleted that exceed quota limit";
			else {
				qInfo() << "Deleted" << devNum << "devices and" << usrNum
						<< "users because their quota exceeded the limit of" << quota;
			}
		}

		Query updateQuotaLimitQuery(db);
		updateQuotaLimitQuery.prepare(QStringLiteral("UPDATE users SET quotalimit = ? "
													 "WHERE quotalimit != ? "
													 "AND quota < ?"));
		updateQuotaLimitQuery.addBindValue(quota);
		updateQuotaLimitQuery.addBindValue(quota);
		updateQuotaLimitQuery.addBindValue(quota);
		updateQuotaLimitQuery.exec();
		auto quotaChanged = updateQuotaLimitQuery.numRowsAffected();
		if(quotaChanged > 0) {
			qInfo() << "Updated quota limit of" << quotaChanged
					<< "users to the new limit" << quota;
		} else
			qDebug() << "No quota changed for any user";

		if(!forceQuota) {
			Query checkQuotaLimitQuery(db);
			checkQuotaLimitQuery.prepare(QStringLiteral("SELECT Count(*) FROM users "
														"WHERE quotalimit != ? "));
			checkQuotaLimitQuery.addBindValue(quota);
			checkQuotaLimitQuery.exec();

			if(!checkQuotaLimitQuery.first())
				throw DatabaseException(db);
			else {
				auto unmatching = checkQuotaLimitQuery.value(0).toULongLong();
				if(unmatching > 0) {
					qWarning() << "Currently" << unmatching << "users cannot be update to new quota"
							   << quota << "because they would exceed that limit.";
				}
			}
		}

		if(!db.commit())
			throw DatabaseException(db);
	} catch(...) {
		db.rollback();
		throw;
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
