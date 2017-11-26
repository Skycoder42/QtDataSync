#include "localstore_p.h"

#include <QtCore/QUrl>
#include <QtCore/QJsonDocument>
#include <QtCore/QUuid>
#include <QtCore/QTemporaryFile>
#include <QtCore/QGlobalStatic>
#include <QtCore/QCoreApplication>
#include <QtCore/QSaveFile>

#include <QtSql/QSqlQuery>

#include <qhashpipe.h>

using namespace QtDataSync;

#define QTDATASYNC_LOG logger

#define EXEC_QUERY(query, key) do {\
	if(!query.exec()) { \
		throw LocalStoreException(defaults, key, query.executedQuery().simplified(), query.lastError().text()); \
	} \
} while(false)

Q_GLOBAL_STATIC(LocalStoreEmitter, emitter)

QReadWriteLock LocalStore::globalLock;

LocalStore::LocalStore(QObject *parent) :
	LocalStore(DefaultSetup, parent)
{}

LocalStore::LocalStore(const QString &setupName, QObject *parent) :
	QObject(parent),
	defaults(setupName),
	logger(defaults.createLogger(staticMetaObject.className(), this)),
	database(defaults.aquireDatabase(this)),
	tableNameCache(),
	dataCache(defaults.property(Defaults::CacheSize).toInt())
{
	connect(emitter, &LocalStoreEmitter::dataChanged,
			this, &LocalStore::onDataChange,
			Qt::DirectConnection);
	connect(emitter, &LocalStoreEmitter::dataResetted,
			this, &LocalStore::onDataReset,
			Qt::DirectConnection);
}

quint64 LocalStore::count(const QByteArray &typeName)
{
	QReadLocker _(&globalLock);

	auto table = getTable(typeName);
	if(table.isEmpty())
		return 0;

	QSqlQuery countQuery(database);
	countQuery.prepare(QStringLiteral("SELECT Count(*) FROM %1").arg(table));
	EXEC_QUERY(countQuery, typeName);

	if(countQuery.first())
		return countQuery.value(0).toInt();
	else
		return 0;
}

QStringList LocalStore::keys(const QByteArray &typeName)
{
	QReadLocker _(&globalLock);

	auto table = getTable(typeName);
	if(table.isEmpty())
		return {};

	QSqlQuery keysQuery(database);
	keysQuery.prepare(QStringLiteral("SELECT Key FROM %1").arg(table));
	EXEC_QUERY(keysQuery, typeName);

	QStringList resList;
	while(keysQuery.next())
		resList.append(keysQuery.value(0).toString());
	return resList;
}

QList<QJsonObject> LocalStore::loadAll(const QByteArray &typeName)
{
	QReadLocker _(&globalLock);

	auto table = getTable(typeName);
	if(table.isEmpty())
		return {};

	QSqlQuery loadQuery(database);
	loadQuery.prepare(QStringLiteral("SELECT Key, File FROM %1").arg(table));
	EXEC_QUERY(loadQuery, typeName);

	QList<QJsonObject> array;//do not cache
	while(loadQuery.next()) {
		int size;
		ObjectKey key {typeName, loadQuery.value(0).toString()};
		auto json = readJson(table, loadQuery.value(1).toString(), key, &size);
		array.append(json);
		dataCache.insert(key, new QJsonObject(json), size);
	}
	return array;
}

QJsonObject LocalStore::load(const ObjectKey &key)
{
	QReadLocker _(&globalLock);

	//check if cached
	auto data = dataCache.object(key);
	if(data)
		return *data;

	auto table = getTable(key.typeName);
	if(table.isEmpty())
		throw NoDataException(defaults, key);

	QSqlQuery loadQuery(database);
	loadQuery.prepare(QStringLiteral("SELECT File FROM %1 WHERE Key = ?").arg(table));
	loadQuery.addBindValue(key.id);
	EXEC_QUERY(loadQuery, key);

	if(loadQuery.first()) {
		int size;
		auto json = readJson(table, loadQuery.value(0).toString(), key, &size);
		dataCache.insert(key, new QJsonObject(json), size);
		return json;
	} else
		throw NoDataException(defaults, key);
}

void LocalStore::save(const ObjectKey &key, const QJsonObject &data)
{
	//scope for the lock
	{
		QWriteLocker _(&globalLock);

		auto table = getTable(key.typeName, true);
		auto tableDir = typeDirectory(table, key);

		if(!database->transaction())
			throw LocalStoreException(defaults, key, database->databaseName(), database->lastError().text());

		try {
			//check if the file exists
			QSqlQuery existQuery(database);
			existQuery.prepare(QStringLiteral("SELECT File FROM %1 WHERE Key = ?").arg(table));
			existQuery.addBindValue(key.id);
			EXEC_QUERY(existQuery, key);

			auto needUpdate = false;
			QScopedPointer<QFileDevice> device;
			std::function<bool(QFileDevice*)> commitFn;

			//create the file device to write to
			if(existQuery.first()) {
				auto file = new QSaveFile(tableDir.absoluteFilePath(existQuery.value(0).toString() + QStringLiteral(".dat")));
				device.reset(file);
				if(!file->open(QIODevice::WriteOnly))
					throw LocalStoreException(defaults, key, file->fileName(), file->errorString());
				commitFn = [](QFileDevice *d){
					return static_cast<QSaveFile*>(d)->commit();
				};
			} else {
				auto fileName = QString::fromUtf8(QUuid::createUuid().toRfc4122().toHex());
				fileName = tableDir.absoluteFilePath(QStringLiteral("%1XXXXXX.dat")).arg(fileName);
				auto file = new QTemporaryFile(fileName);
				device.reset(file);
				if(!file->open())
					throw LocalStoreException(defaults, key, file->fileName(), file->errorString());
				needUpdate = true;
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
			hashPipe.setAutoClose(false);
			hashPipe.pipeTo(device.data());
			hashPipe.pipeWrite(doc.toBinaryData());
			hashPipe.pipeClose();
			if(device->error() != QFile::NoError) //TODO commit AFTER updating the database?
				throw LocalStoreException(defaults, key, device->fileName(), device->errorString());

			//save key in database
			QFileInfo info(device->fileName());
			if(needUpdate) {//TODO always update, because of id and hash...
				QSqlQuery insertQuery(database);
				insertQuery.prepare(QStringLiteral("INSERT INTO %1 (Key, File, Checksum) VALUES(?, ?, ?)").arg(table));
				insertQuery.addBindValue(key.id);
				insertQuery.addBindValue(tableDir.relativeFilePath(info.completeBaseName()));
				insertQuery.addBindValue(hashPipe.hash().toHex());
				EXEC_QUERY(insertQuery, key);
			} //TODO ...via else too

			//complete the file-save
			if(!commitFn(device.data()))
				throw LocalStoreException(defaults, key, device->fileName(), device->errorString());

			if(!database->commit())
				throw LocalStoreException(defaults, key, database->databaseName(), database->lastError().text());

			//update cache
			dataCache.insert(key, new QJsonObject(data), info.size());

			//notify others
			emit emitter->dataChanged(this, key, data, info.size());
		} catch(...) {
			database->rollback();
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
		QWriteLocker _(&globalLock);

		auto table = getTable(key.typeName);
		if(table.isEmpty()) //no table -> nothing to delete
			return false;

		if(!database->transaction())
			throw LocalStoreException(defaults, key, database->databaseName(), database->lastError().text());

		try {
			QSqlQuery loadQuery(database);
			loadQuery.prepare(QStringLiteral("SELECT File FROM %1 WHERE Key = ?").arg(table));
			loadQuery.addBindValue(key.id);
			EXEC_QUERY(loadQuery, key);

			if(loadQuery.first()) {
				QSqlQuery removeQuery(database);
				removeQuery.prepare(QStringLiteral("DELETE FROM %1 WHERE Key = ?").arg(table));
				removeQuery.addBindValue(key.id);
				EXEC_QUERY(removeQuery, key);

				auto tableDir = typeDirectory(table, key);
				auto fileName = tableDir.absoluteFilePath(loadQuery.value(0).toString() + QStringLiteral(".dat"));
				if(!QFile::remove(fileName))
					throw LocalStoreException(defaults, key, fileName, QStringLiteral("Failed to delete file"));

				if(!database->commit())
					throw LocalStoreException(defaults, key, database->databaseName(), database->lastError().text());

				//update cache
				dataCache.remove(key);

				//notify others
				emit emitter->dataChanged(this, key, {}, 0);
			} else
				return false;
		} catch(...) {
			database->rollback();
			throw;
		}
	}

	//own signal
	emit dataChanged(key, true);
	return true;
}

QList<QJsonObject> LocalStore::find(const QByteArray &typeName, const QString &query)
{
	QReadLocker _(&globalLock);

	auto table = getTable(typeName);
	if(table.isEmpty())
		return {};

	auto searchQuery = query;
	searchQuery.replace(QLatin1Char('*'), QLatin1Char('%'));
	searchQuery.replace(QLatin1Char('?'), QLatin1Char('_'));

	QSqlQuery findQuery(database);
	findQuery.prepare(QStringLiteral("SELECT Key, File FROM %1 WHERE Key LIKE ?").arg(table));
	findQuery.addBindValue(searchQuery);
	EXEC_QUERY(findQuery, typeName);

	QList<QJsonObject> array;
	while(findQuery.next()) {
		int size;
		ObjectKey key {typeName, findQuery.value(0).toString()};
		auto json = readJson(table, findQuery.value(1).toString(), key, &size);
		array.append(json);
		dataCache.insert(key, new QJsonObject(json), size);
	}
	return array;
}

void LocalStore::clear(const QByteArray &typeName)
{
	//scope for the lock
	{
		QWriteLocker _(&globalLock);

		auto table = getTable(typeName);
		if(table.isEmpty()) //no table -> nothing to delete
			return;

		QSqlQuery dropQuery(database);
		dropQuery.prepare(QStringLiteral("DROP TABLE %1").arg(table));
		EXEC_QUERY(dropQuery, typeName);

		auto tableDir = typeDirectory(table, typeName);
		if(!tableDir.removeRecursively())
			throw LocalStoreException(defaults, typeName, tableDir.absolutePath(), QStringLiteral("Failed to delete cleared data directory"));

		//notify others (and self)
		emit emitter->dataResetted(this, typeName);
	}

	//own signal
	emit dataCleared(typeName);
}

void LocalStore::reset()
{
	//scope for the lock
	{
		QWriteLocker _(&globalLock);

		if(!database->transaction())
			throw LocalStoreException(defaults, {}, database->databaseName(), database->lastError().text());

		try {
			foreach(auto table, database->tables()) {
				if(!table.startsWith(QStringLiteral("data_")))
					continue;
				QSqlQuery dropQuery(database);
				dropQuery.prepare(QStringLiteral("DROP TABLE %1").arg(table));
				EXEC_QUERY(dropQuery, {});
			}
		} catch(...) {
			database->rollback();
			throw;
		}

		auto tableDir = defaults.storageDir();
		if(tableDir.cd(QStringLiteral("store"))) {
			if(!tableDir.removeRecursively()) //no rollback, as partially removed is possible, better keep junk data...
				throw LocalStoreException(defaults, {}, tableDir.absolutePath(), QStringLiteral("Failed to delete data directory"));
		}

		//notify others (and self)
		emit emitter->dataResetted(this);
	}

	//own signal
	emit dataResetted();
}

void LocalStore::onDataChange(QObject *origin, const ObjectKey &key, const QJsonObject &data, int size)
{
	if(origin != this) {
		if(dataCache.contains(key)) {
			if(data.isEmpty())
				dataCache.remove(key);
			else
				dataCache.insert(key, new QJsonObject(data), size);
		}

		QMetaObject::invokeMethod(this, "dataChanged", Qt::QueuedConnection,
								  Q_ARG(ObjectKey, key),
								  Q_ARG(bool, data.isEmpty()));
	}
}

void LocalStore::onDataReset(QObject *origin, const QByteArray &typeName)
{
	if(typeName.isNull()) {
		tableNameCache.clear();
		dataCache.clear();

		if(origin != this)
			QMetaObject::invokeMethod(this, "dataResetted", Qt::QueuedConnection);
	} else {
		tableNameCache.remove(typeName);
		dataCache.clear();//TODO iterate? may take significantly longer!

		if(origin != this) {
			QMetaObject::invokeMethod(this, "dataCleared", Qt::QueuedConnection,
									  Q_ARG(QByteArray, typeName));
		}
	}
}

QString LocalStore::getTable(const QByteArray &typeName, bool allowCreate)
{
	//create table
	if(!tableNameCache.contains(typeName)) {
		auto encName = QUrl::toPercentEncoding(QString::fromUtf8(typeName))
					   .replace('%', '_');
		auto tableName = QStringLiteral("data_%1")
						 .arg(QString::fromUtf8(encName));

		if(!database->tables().contains(tableName)) {
			if(allowCreate) {
				QSqlQuery createQuery(database);
				createQuery.prepare(QStringLiteral("CREATE TABLE %1 ("
												   "Key			TEXT NOT NULL,"
												   "Version		INTEGER NOT NULL DEFAULT 1,"
												   "File		TEXT NOT NULL,"
												   "Checksum	BLOB NOT NULL,"
												   "PRIMARY KEY(Key)"
												   ");").arg(tableName));
				EXEC_QUERY(createQuery, typeName);

				tableNameCache.insert(typeName, tableName);
			}
		} else
			tableNameCache.insert(typeName, tableName);
	}

	return tableNameCache.value(typeName);
}

QDir LocalStore::typeDirectory(const QString &tableName, const ObjectKey &key)
{
	auto tName = QStringLiteral("store/%1").arg(tableName);
	auto tableDir = defaults.storageDir();
	if(!tableDir.mkpath(tName) || !tableDir.cd(tName)) {
		throw LocalStoreException(defaults, key, tName, QStringLiteral("Failed to create directory"));
	} else
		return tableDir;
}

QJsonObject LocalStore::readJson(const QString &tableName, const QString &fileName, const ObjectKey &key, int *costs)
{
	QFile file(typeDirectory(tableName, key).absoluteFilePath(fileName + QStringLiteral(".dat")));
	if(!file.open(QIODevice::ReadOnly))
		throw LocalStoreException(defaults, key, file.fileName(), file.errorString());

	auto doc = QJsonDocument::fromBinaryData(file.readAll());
	if(costs)
		*costs = file.size();
	file.close();

	if(!doc.isObject())
		throw LocalStoreException(defaults, key, file.fileName(), QStringLiteral("File contains invalid json data"));
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


// ------------- Exceptions -------------

LocalStoreException::LocalStoreException(const Defaults &defaults, const ObjectKey &key, const QString &context, const QString &message) :
	Exception(defaults, message),
	_key(key),
	_context(context)
{}

LocalStoreException::LocalStoreException(const LocalStoreException * const other) :
	Exception(other),
	_key(other->_key),
	_context(other->_context)
{}

ObjectKey LocalStoreException::key() const
{
	return _key;
}

QString LocalStoreException::context() const
{
	return _context;
}

QString LocalStoreException::qWhat() const
{
	QString msg = Exception::qWhat() +
				  QStringLiteral("\n\tContext: %1"
								 "\n\tTypeName: %2")
				  .arg(_context)
				  .arg(QString::fromUtf8(_key.typeName));
	if(!_key.id.isNull())
		msg += QStringLiteral("\n\tKey: %1").arg(_key.id);
	return msg;
}

void LocalStoreException::raise() const
{
	throw (*this);
}

QException *LocalStoreException::clone() const
{
	return new LocalStoreException(this);
}



NoDataException::NoDataException(const Defaults &defaults, const ObjectKey &key) :
	Exception(defaults, QStringLiteral("The requested data does not exist")),
	_key(key)
{}

NoDataException::NoDataException(const NoDataException * const other) :
	Exception(other),
	_key(other->_key)
{}

ObjectKey NoDataException::key() const
{
	return _key;
}

QString NoDataException::qWhat() const
{
	return Exception::qWhat() +
			QStringLiteral("\n\tTypeName: %1"
						   "\n\tKey: %2")
			.arg(QString::fromUtf8(_key.typeName))
			.arg(_key.id);
}

void NoDataException::raise() const
{
	throw (*this);
}

QException *NoDataException::clone() const
{
	return new NoDataException(this);
}
