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

	//make shure the primary table exists
	_database.createGlobalScheme(_defaults);
}

LocalStore::~LocalStore() {}

quint64 LocalStore::count(const QByteArray &typeName)
{
	QReadLocker _(_defaults.databaseLock());

	QSqlQuery countQuery(_database);
	countQuery.prepare(QStringLiteral("SELECT Count(Id) FROM DataIndex WHERE Type = ? AND File IS NOT NULL"));
	countQuery.addBindValue(typeName);
	exec(countQuery, typeName); //TODO add special method for prepare as well

	if(countQuery.first())
		return countQuery.value(0).toInt();
	else
		return 0;
}

QStringList LocalStore::keys(const QByteArray &typeName)
{
	QReadLocker _(_defaults.databaseLock());

	QSqlQuery keysQuery(_database);
	keysQuery.prepare(QStringLiteral("SELECT Key FROM DataIndex WHERE Type = ? AND File IS NOT NULL"));
	keysQuery.addBindValue(typeName);
	exec(keysQuery, typeName);

	QStringList resList;
	while(keysQuery.next())
		resList.append(keysQuery.value(0).toString());
	return resList;
}

QList<QJsonObject> LocalStore::loadAll(const QByteArray &typeName)
{
	QReadLocker _(_defaults.databaseLock());

	QSqlQuery loadQuery(_database);
	loadQuery.prepare(QStringLiteral("SELECT Key, File FROM DataIndex WHERE Type = ? AND File IS NOT NULL"));
	loadQuery.addBindValue(typeName);
	exec(loadQuery, typeName);

	QList<QJsonObject> array;
	while(loadQuery.next()) {
		int size;
		ObjectKey key {typeName, loadQuery.value(0).toString()};
		auto json = readJson(key, loadQuery.value(1).toString(), &size);
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

	QSqlQuery loadQuery(_database);
	loadQuery.prepare(QStringLiteral("SELECT File FROM DataIndex WHERE Type = ? AND Key = ? AND File IS NOT NULL"));
	loadQuery.addBindValue(key.typeName);
	loadQuery.addBindValue(key.id);
	exec(loadQuery, key);

	if(loadQuery.first()) {
		int size;
		auto json = readJson(key, loadQuery.value(0).toString(), &size);
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

		auto tableDir = typeDirectory(key);

		if(!_database->transaction())
			throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());

		try {
			//check if the file exists
			QSqlQuery existQuery(_database);
			existQuery.prepare(QStringLiteral("SELECT Version, File FROM DataIndex WHERE Type = ? AND Key = ?"));
			existQuery.addBindValue(key.typeName);
			existQuery.addBindValue(key.id);
			exec(existQuery, key);

			QScopedPointer<QFileDevice> device;
			std::function<bool(QFileDevice*)> commitFn;

			//create the file device to write to
			quint64 version = 1ull;
			bool existing = existQuery.first();
			if(existing)
				version = existQuery.value(0).toULongLong() + 1ull;

			if(existing && !existQuery.value(1).isNull()) {
				auto file = new QSaveFile(tableDir.absoluteFilePath(existQuery.value(1).toString() + QStringLiteral(".dat")));
				device.reset(file);
				if(!file->open(QIODevice::WriteOnly))
					throw LocalStoreException(_defaults, key, file->fileName(), file->errorString());
				commitFn = [](QFileDevice *d){
					return static_cast<QSaveFile*>(d)->commit();
				};
			} else {
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
			if(existing) {
				QSqlQuery updateQuery(_database);
				updateQuery.prepare(QStringLiteral("UPDATE DataIndex SET Version = ?, File = ?, Checksum = ?, Changed = 1 WHERE Type = ? AND Key = ?"));
				updateQuery.addBindValue(version);
				updateQuery.addBindValue(tableDir.relativeFilePath(info.completeBaseName())); //still update file, in case it was set to NULL
				updateQuery.addBindValue(hashPipe.hash());
				updateQuery.addBindValue(key.typeName);
				updateQuery.addBindValue(key.id);
				exec(updateQuery, key);
			} else {
				QSqlQuery insertQuery(_database);
				insertQuery.prepare(QStringLiteral("INSERT INTO DataIndex (Type, Key, Version, File, Checksum) VALUES(?, ?, ?, ?, ?)"));
				insertQuery.addBindValue(key.typeName);
				insertQuery.addBindValue(key.id);
				insertQuery.addBindValue(version);
				insertQuery.addBindValue(tableDir.relativeFilePath(info.completeBaseName()));
				insertQuery.addBindValue(hashPipe.hash());
				exec(insertQuery, key);
			}

			//notify change controller
			ChangeController::triggerDataChange(_defaults, _);

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

		if(!_database->transaction())
			throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());

		try {
			//load data of existing entry
			QSqlQuery loadQuery(_database);
			loadQuery.prepare(QStringLiteral("SELECT Version, File FROM DataIndex WHERE Type = ? AND Key = ? AND File IS NOT NULL"));
			loadQuery.addBindValue(key.typeName);
			loadQuery.addBindValue(key.id);
			exec(loadQuery, key);

			if(loadQuery.first()) { //stored -> remove it
				auto version = loadQuery.value(0).toULongLong() + 1;
				auto tableDir = typeDirectory(key);
				auto fileName = tableDir.absoluteFilePath(loadQuery.value(1).toString() + QStringLiteral(".dat"));

				//"remove" from db
				QSqlQuery removeQuery(_database);
				removeQuery.prepare(QStringLiteral("UPDATE DataIndex SET Version = ?, File = NULL, Checksum = NULL, Changed = 1 WHERE Type = ? AND Key = ?"));
				removeQuery.addBindValue(version);
				removeQuery.addBindValue(key.typeName);
				removeQuery.addBindValue(key.id);
				exec(removeQuery, key);

				//notify change controller
				ChangeController::triggerDataChange(_defaults, _);

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

	auto searchQuery = query;
	searchQuery.replace(QLatin1Char('*'), QLatin1Char('%'));
	searchQuery.replace(QLatin1Char('?'), QLatin1Char('_'));

	QSqlQuery findQuery(_database);
	findQuery.prepare(QStringLiteral("SELECT Key, File FROM DataIndex WHERE Type = ? AND Key LIKE ? AND File IS NOT NULL"));
	findQuery.addBindValue(typeName);
	findQuery.addBindValue(searchQuery);
	exec(findQuery, typeName);

	QList<QJsonObject> array;
	while(findQuery.next()) {
		int size;
		ObjectKey key {typeName, findQuery.value(0).toString()};
		auto json = readJson(key, findQuery.value(1).toString(), &size);
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

		if(!_database->transaction())
			throw LocalStoreException(_defaults, typeName, _database->databaseName(), _database->lastError().text());

		try {
			QSqlQuery clearQuery(_database);
			clearQuery.prepare(QStringLiteral("DELETE FROM DataIndex WHERE Type = ?"));
			clearQuery.addBindValue(typeName);
			exec(clearQuery, typeName);

			//notify change controller
			ChangeController::triggerDataClear(_defaults, _database, typeName, _);

			auto tableDir = typeDirectory(typeName);
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
			QSqlQuery resetQuery(_database);
			resetQuery.prepare(QStringLiteral("DELETE FROM DataIndex"));
			exec(resetQuery);

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

QDir LocalStore::typeDirectory(const ObjectKey &key)
{
	auto encName = QUrl::toPercentEncoding(QString::fromUtf8(key.typeName))
				   .replace('%', '_');
	auto tName = QStringLiteral("store/data_%1").arg(QString::fromUtf8(encName));
	auto tableDir = _defaults.storageDir();
	if(!tableDir.mkpath(tName) || !tableDir.cd(tName)) {
		throw LocalStoreException(_defaults, key, tName, QStringLiteral("Failed to create directory"));
	} else
		return tableDir;
}

QJsonObject LocalStore::readJson(const ObjectKey &key, const QString &fileName, int *costs)
{
	QFile file(typeDirectory(key).absoluteFilePath(fileName + QStringLiteral(".dat")));
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
