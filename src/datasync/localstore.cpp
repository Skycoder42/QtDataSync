#include "localstore_p.h"
#include "datastore.h"
#include "changecontroller_p.h"

#include <QtCore/QUrl>
#include <QtCore/QJsonDocument>
#include <QtCore/QUuid>
#include <QtCore/QTemporaryFile>
#include <QtCore/QGlobalStatic>
#include <QtCore/QCoreApplication>
#include <QtCore/QSaveFile>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <qhashpipe.h>

using namespace QtDataSync;

#define QTDATASYNC_LOG _logger

Q_GLOBAL_STATIC(LocalStoreEmitter, emitter)

LocalStore::LocalStore(QObject *parent) :
	LocalStore(DefaultSetup, parent)
{}

LocalStore::LocalStore(const QString &setupName, QObject *parent) :
	QObject(parent),
	_defaults(setupName),
	_logger(_defaults.createLogger(staticMetaObject.className(), this)),
	_database(_defaults.aquireDatabase(this)),
	_tableNameCache(),
	_dataCache(_defaults.property(Defaults::CacheSize).toInt())
{
	connect(emitter, &LocalStoreEmitter::dataChanged,
			this, &LocalStore::onDataChange,
			Qt::DirectConnection);
	connect(emitter, &LocalStoreEmitter::dataResetted,
			this, &LocalStore::onDataReset,
			Qt::DirectConnection);
}

LocalStore::~LocalStore() {}

quint64 LocalStore::count(const QByteArray &typeName)
{
	QReadLocker _(_defaults.databaseLock());

	auto table = getTable(typeName);
	if(table.isEmpty())
		return 0;

	QSqlQuery countQuery(_database);
	countQuery.prepare(QStringLiteral("SELECT Count(Key) FROM %1").arg(table));
	exec(countQuery, typeName);

	if(countQuery.first())
		return countQuery.value(0).toInt();
	else
		return 0;
}

QStringList LocalStore::keys(const QByteArray &typeName)
{
	QReadLocker _(_defaults.databaseLock());

	auto table = getTable(typeName);
	if(table.isEmpty())
		return {};

	QSqlQuery keysQuery(_database);
	keysQuery.prepare(QStringLiteral("SELECT Key FROM %1").arg(table));
	exec(keysQuery, typeName);

	QStringList resList;
	while(keysQuery.next())
		resList.append(keysQuery.value(0).toString());
	return resList;
}

QList<QJsonObject> LocalStore::loadAll(const QByteArray &typeName)
{
	QReadLocker _(_defaults.databaseLock());

	auto table = getTable(typeName);
	if(table.isEmpty())
		return {};

	QSqlQuery loadQuery(_database);
	loadQuery.prepare(QStringLiteral("SELECT Key, File FROM %1").arg(table));
	exec(loadQuery, typeName);

	QList<QJsonObject> array;
	while(loadQuery.next()) {
		int size;
		ObjectKey key {typeName, loadQuery.value(0).toString()};
		auto json = readJson(table, loadQuery.value(1).toString(), key, &size);
		array.append(json);
		_dataCache.insert(key, new QJsonObject(json), size);
	}
	return array;
}

QJsonObject LocalStore::load(const ObjectKey &key)
{
	QReadLocker _(_defaults.databaseLock());

	//check if cached
	auto data = _dataCache.object(key);
	if(data)
		return *data;

	auto table = getTable(key.typeName);
	if(table.isEmpty())
		throw NoDataException(_defaults, key);

	QSqlQuery loadQuery(_database);
	loadQuery.prepare(QStringLiteral("SELECT File FROM %1 WHERE Key = ?").arg(table));
	loadQuery.addBindValue(key.id);
	exec(loadQuery, key);

	if(loadQuery.first()) {
		int size;
		auto json = readJson(table, loadQuery.value(0).toString(), key, &size);
		_dataCache.insert(key, new QJsonObject(json), size);
		return json;
	} else
		throw NoDataException(_defaults, key);
}

void LocalStore::save(const ObjectKey &key, const QJsonObject &data)
{
	//scope for the lock
	{
		QWriteLocker _(_defaults.databaseLock());

		auto table = getTable(key.typeName, true);
		auto tableDir = typeDirectory(table, key);

		if(!_database->transaction())
			throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());

		try {
			//check if the file exists
			QSqlQuery existQuery(_database);
			existQuery.prepare(QStringLiteral("SELECT Version, File FROM %1 WHERE Key = ?").arg(table));
			existQuery.addBindValue(key.id);
			exec(existQuery, key);

			auto notExisting = false;
			QScopedPointer<QFileDevice> device;
			std::function<bool(QFileDevice*)> commitFn;

			//create the file device to write to
			quint64 version = 1ull;
			if(existQuery.first()) {
				version = existQuery.value(0).toULongLong() + 1ull;

				auto file = new QSaveFile(tableDir.absoluteFilePath(existQuery.value(1).toString() + QStringLiteral(".dat")));
				device.reset(file);
				if(!file->open(QIODevice::WriteOnly))
					throw LocalStoreException(_defaults, key, file->fileName(), file->errorString());
				commitFn = [](QFileDevice *d){
					return static_cast<QSaveFile*>(d)->commit();
				};
			} else {
				notExisting = true;

				auto fileName = QString::fromUtf8(QUuid::createUuid().toRfc4122().toHex());
				fileName = tableDir.absoluteFilePath(QStringLiteral("%1XXXXXX.dat")).arg(fileName);
				auto file = new QTemporaryFile(fileName);
				device.reset(file);
				if(!file->open())
					throw LocalStoreException(_defaults, key, file->fileName(), file->errorString());
				commitFn = [](QFileDevice *d){
					auto f = static_cast<QTemporaryFile*>(d);
					f->close();
					if(f->error() == QFile::NoError) {
						f->setAutoRemove(false);
						return true;
					} else
						return false;
				};
			}

			//write the data & get the hash
			QJsonDocument doc(data);
			QHashPipe hashPipe(QCryptographicHash::Sha3_256);
			hashPipe.setAutoOpen(false);
			hashPipe.setAutoClose(false);
			hashPipe.pipeTo(device.data());
			hashPipe.open();
			hashPipe.write(doc.toBinaryData());
			hashPipe.close();
			if(device->error() != QFile::NoError)
				throw LocalStoreException(_defaults, key, device->fileName(), device->errorString());

			//save key in database
			QFileInfo info(device->fileName());
			if(notExisting) {
				QSqlQuery insertQuery(_database);
				insertQuery.prepare(QStringLiteral("INSERT INTO %1 (Key, Version, File, Checksum) VALUES(?, ?, ?, ?)").arg(table));
				insertQuery.addBindValue(key.id);
				insertQuery.addBindValue(version);
				insertQuery.addBindValue(tableDir.relativeFilePath(info.completeBaseName()));
				insertQuery.addBindValue(hashPipe.hash());
				exec(insertQuery, key);
			} else {
				QSqlQuery updateQuery(_database);
				updateQuery.prepare(QStringLiteral("UPDATE %1 SET Version = ?, Checksum = ? WHERE Key = ?").arg(table));
				updateQuery.addBindValue(version);
				updateQuery.addBindValue(hashPipe.hash());
				updateQuery.addBindValue(key.id);
				exec(updateQuery, key);
			}

			//notify change controller
			ChangeController::triggerDataChange(_defaults, _database, {key, version, hashPipe.hash()}, _);

			//complete the file-save (last before commit!)
			if(!commitFn(device.data()))
				throw LocalStoreException(_defaults, key, device->fileName(), device->errorString());

			//commit database changes
			if(!_database->commit())
				throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());

			//update cache
			_dataCache.insert(key, new QJsonObject(data), info.size());

			//notify others
			emit emitter->dataChanged(this, key, data, info.size());
		} catch(...) {
			_database->rollback();
			throw;
		}
	}

	//own signal
	emit dataChanged(key, false);
}

bool LocalStore::remove(const ObjectKey &key)
{
	//scope for the lock
	{
		QWriteLocker _(_defaults.databaseLock());

		auto table = getTable(key.typeName);
		if(table.isEmpty()) //no table -> nothing to delete
			return false;

		if(!_database->transaction())
			throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());

		try {
			//load data of existing entry
			QSqlQuery loadQuery(_database);
			loadQuery.prepare(QStringLiteral("SELECT Version, File FROM %1 WHERE Key = ?").arg(table));
			loadQuery.addBindValue(key.id);
			exec(loadQuery, key);

			if(loadQuery.first()) { //stored -> remove it
				auto version = loadQuery.value(0).toULongLong() + 1;
				auto tableDir = typeDirectory(table, key);
				auto fileName = tableDir.absoluteFilePath(loadQuery.value(1).toString() + QStringLiteral(".dat"));

				//remove from db
				QSqlQuery removeQuery(_database);
				removeQuery.prepare(QStringLiteral("DELETE FROM %1 WHERE Key = ?").arg(table));
				removeQuery.addBindValue(key.id);
				exec(removeQuery, key);

				//notify change controller
				ChangeController::triggerDataChange(_defaults, _database, {key, version}, _);

				//delete the file
				QFile rmFile(fileName);
				if(!rmFile.remove())
					throw LocalStoreException(_defaults, key, rmFile.fileName(), rmFile.errorString());

				//commit db
				if(!_database->commit())
					throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());

				//update cache
				_dataCache.remove(key);

				//notify others
				emit emitter->dataChanged(this, key, {}, 0);
			} else { //not stored -> done
				if(!_database->commit())
					throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());

				return false;
			}
		} catch(...) {
			_database->rollback();
			throw;
		}
	}

	//own signal
	emit dataChanged(key, true);
	return true;
}

QList<QJsonObject> LocalStore::find(const QByteArray &typeName, const QString &query)
{
	QReadLocker _(_defaults.databaseLock());

	auto table = getTable(typeName);
	if(table.isEmpty())
		return {};

	auto searchQuery = query;
	searchQuery.replace(QLatin1Char('*'), QLatin1Char('%'));
	searchQuery.replace(QLatin1Char('?'), QLatin1Char('_'));

	QSqlQuery findQuery(_database);
	findQuery.prepare(QStringLiteral("SELECT Key, File FROM %1 WHERE Key LIKE ?").arg(table));
	findQuery.addBindValue(searchQuery);
	exec(findQuery, typeName);

	QList<QJsonObject> array;
	while(findQuery.next()) {
		int size;
		ObjectKey key {typeName, findQuery.value(0).toString()};
		auto json = readJson(table, findQuery.value(1).toString(), key, &size);
		array.append(json);
		_dataCache.insert(key, new QJsonObject(json), size);
	}
	return array;
}

void LocalStore::clear(const QByteArray &typeName)
{
	//scope for the lock
	{
		QWriteLocker _(_defaults.databaseLock());

		auto table = getTable(typeName);
		if(table.isEmpty()) //no table -> nothing to delete
			return;

		if(!_database->transaction())
			throw LocalStoreException(_defaults, typeName, _database->databaseName(), _database->lastError().text());

		try {
			QSqlQuery dropQuery(_database);
			dropQuery.prepare(QStringLiteral("DROP TABLE IF EXISTS %1").arg(table));
			exec(dropQuery, typeName);

			//notify change controller
			ChangeController::triggerDataClear(_defaults, _database, typeName, _);

			auto tableDir = typeDirectory(table, typeName);
			if(!tableDir.removeRecursively()) {
				logWarning() << "Failed to delete cleared data directory for type"
							 << typeName;
			}

			if(!_database->commit())
				throw LocalStoreException(_defaults, typeName, _database->databaseName(), _database->lastError().text());

			//notify others (and self)
			emit emitter->dataResetted(this, typeName);
		} catch(...) {
			_database->rollback();
			throw;
		}
	}

	//own signal
	emit dataCleared(typeName);
}

void LocalStore::reset()
{
	//scope for the lock
	{
		QWriteLocker _(_defaults.databaseLock());

		if(!_database->transaction())
			throw LocalStoreException(_defaults, {}, _database->databaseName(), _database->lastError().text());

		try {
			foreach(auto table, _database->tables()) {
				if(!table.startsWith(QStringLiteral("data_")))
					continue;
				QSqlQuery dropQuery(_database);
				dropQuery.prepare(QStringLiteral("DROP TABLE %1").arg(table));
				exec(dropQuery);
			}

			//note: resets are local only, so they dont trigger any changecontroller stuff

			auto tableDir = _defaults.storageDir();
			if(tableDir.cd(QStringLiteral("store"))) {
				if(!tableDir.removeRecursively()) //no rollback, as partially removed is possible, better keep junk data...
					logWarning() << "Failed to delete store directory" << tableDir.absolutePath();
			}

			if(!_database->commit())
				throw LocalStoreException(_defaults, QByteArray("<any>"), _database->databaseName(), _database->lastError().text());
		} catch(...) {
			_database->rollback();
			throw;
		}

		//notify others (and self)
		emit emitter->dataResetted(this);
	}

	//own signal
	emit dataResetted();
}

int LocalStore::cacheSize() const
{
	return _dataCache.maxCost();
}

void LocalStore::setCacheSize(int cacheSize)
{
	_dataCache.setMaxCost(cacheSize);
}

void LocalStore::resetCacheSize()
{
	_dataCache.setMaxCost(_defaults.property(Defaults::CacheSize).toInt());
}

void LocalStore::onDataChange(QObject *origin, const ObjectKey &key, const QJsonObject &data, int size)
{
	if(origin != this) {
		if(_dataCache.contains(key)) {
			if(data.isEmpty())
				_dataCache.remove(key);
			else
				_dataCache.insert(key, new QJsonObject(data), size);
		}

		QMetaObject::invokeMethod(this, "dataChanged", Qt::QueuedConnection,
								  Q_ARG(QtDataSync::ObjectKey, key),
								  Q_ARG(bool, data.isEmpty()));
	}
}

void LocalStore::onDataReset(QObject *origin, const QByteArray &typeName)
{
	if(typeName.isNull()) {
		_tableNameCache.clear();
		_dataCache.clear();

		if(origin != this)
			QMetaObject::invokeMethod(this, "dataResetted", Qt::QueuedConnection);
	} else {
		_tableNameCache.remove(typeName);
		foreach(auto key, _dataCache.keys()) {
			if(key.typeName == typeName)
				_dataCache.remove(key);
		}

		if(origin != this) {
			QMetaObject::invokeMethod(this, "dataCleared", Qt::QueuedConnection,
									  Q_ARG(QByteArray, typeName));
		}
	}
}

void LocalStore::exec(QSqlQuery &query, const ObjectKey &key) const
{
	if(!query.exec()) {
		throw LocalStoreException(_defaults,
								  key,
								  query.executedQuery().simplified(),
								  query.lastError().text());
	}
}

QString LocalStore::getTable(const QByteArray &typeName, bool allowCreate)
{
	//create table
	if(!_tableNameCache.contains(typeName)) {
		auto encName = QUrl::toPercentEncoding(QString::fromUtf8(typeName))
					   .replace('%', '_');
		auto tableName = QStringLiteral("data_%1")
						 .arg(QString::fromUtf8(encName));

		if(!_database->tables().contains(tableName)) {
			if(allowCreate) {
				QSqlQuery createQuery(_database);
				createQuery.prepare(QStringLiteral("CREATE TABLE %1 ("
												   "Key			TEXT NOT NULL,"
												   "Version		INTEGER NOT NULL,"
												   "File		TEXT NOT NULL,"
												   "Checksum	BLOB NOT NULL,"
												   "PRIMARY KEY(Key)"
												   ");").arg(tableName));
				exec(createQuery, typeName);

				_tableNameCache.insert(typeName, tableName);
			}
		} else
			_tableNameCache.insert(typeName, tableName);
	}

	//to be save: check the same for changecontroller
	ChangeController::createTables(_defaults, _database, allowCreate);

	return _tableNameCache.value(typeName);
}

QDir LocalStore::typeDirectory(const QString &tableName, const ObjectKey &key)
{
	auto tName = QStringLiteral("store/%1").arg(tableName);
	auto tableDir = _defaults.storageDir();
	if(!tableDir.mkpath(tName) || !tableDir.cd(tName)) {
		throw LocalStoreException(_defaults, key, tName, QStringLiteral("Failed to create directory"));
	} else
		return tableDir;
}

QJsonObject LocalStore::readJson(const QString &tableName, const QString &fileName, const ObjectKey &key, int *costs)
{
	QFile file(typeDirectory(tableName, key).absoluteFilePath(fileName + QStringLiteral(".dat")));
	if(!file.open(QIODevice::ReadOnly))
		throw LocalStoreException(_defaults, key, file.fileName(), file.errorString());

	auto doc = QJsonDocument::fromBinaryData(file.readAll());
	if(costs)
		*costs = file.size();
	file.close();

	if(!doc.isObject())
		throw LocalStoreException(_defaults, key, file.fileName(), QStringLiteral("File contains invalid json data"));
	return doc.object();
}

// ------------- Emitter -------------

LocalStoreEmitter::LocalStoreEmitter(QObject *parent) :
	QObject(parent)
{
	auto coreThread = qApp->thread();
	if(thread() != coreThread)
		moveToThread(coreThread);
}
