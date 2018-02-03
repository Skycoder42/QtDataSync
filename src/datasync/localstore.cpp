#include "localstore_p.h"
#include "changecontroller_p.h"
#include "synchelper_p.h"
#include "emitteradapter_p.h"

#include <QtCore/QUrl>
#include <QtCore/QJsonDocument>
#include <QtCore/QTemporaryFile>
#include <QtCore/QCoreApplication>
#include <QtCore/QSaveFile>
#include <QtCore/QRegularExpression>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

using namespace QtDataSync;
using std::function;
using std::tuple;
using std::make_tuple;

#define QTDATASYNC_LOG _logger
#define SCOPE_ASSERT() Q_ASSERT_X(scope.d->database.isValid(), Q_FUNC_INFO, "Cannot use SyncScope after committing it")

LocalStore::LocalStore(const Defaults &defaults, QObject *parent) :
	QObject(parent),
	_defaults(defaults),
	_logger(_defaults.createLogger("store", this)),
	_emitter(_defaults.createEmitter(this)),
	_database(_defaults.aquireDatabase(this))
{
	connect(_emitter, &EmitterAdapter::dataChanged,
			this, &LocalStore::dataChanged);
	connect(_emitter, &EmitterAdapter::dataCleared,
			this, &LocalStore::dataCleared);
	connect(_emitter, &EmitterAdapter::dataResetted,
			this, &LocalStore::dataResetted);

	if(!_database->tables().contains(QStringLiteral("DataIndex"))) {
		QSqlQuery createQuery(_database);
		createQuery.prepare(QStringLiteral("CREATE TABLE IF NOT EXISTS DataIndex ("
										   "	Type		TEXT NOT NULL,"
										   "	Id			TEXT NOT NULL,"
										   "	Version		INTEGER NOT NULL,"
										   "	File		TEXT,"
										   "	Checksum	BLOB,"
										   "	Changed		INTEGER NOT NULL DEFAULT 1,"
										   "	PRIMARY KEY(Type, Id)"
										   ") WITHOUT ROWID;"));
		if(!createQuery.exec()) {
			throw LocalStoreException(_defaults,
									  QByteArrayLiteral("any"),
									  createQuery.executedQuery().simplified(),
									  createQuery.lastError().text());
		}
		logDebug() << "Created DataIndex table";
	}

	if(!_database->tables().contains(QStringLiteral("DeviceUploads"))) {
		QSqlQuery createQuery(_database);
		createQuery.prepare(QStringLiteral("CREATE TABLE IF NOT EXISTS DeviceUploads ( "
										   "	Type	TEXT NOT NULL, "
										   "	Id		TEXT NOT NULL, "
										   "	Device	TEXT NOT NULL, "
										   "	PRIMARY KEY(Type, Id, Device), "
										   "	FOREIGN KEY(Type, Id) REFERENCES DataIndex ON DELETE CASCADE "
										   ") WITHOUT ROWID;"));
		if(!createQuery.exec()) {
			throw LocalStoreException(_defaults,
									  QByteArrayLiteral("any"),
									  createQuery.executedQuery().simplified(),
									  createQuery.lastError().text());
		}
		logDebug() << "Created DeviceUploads table";
	}
}

LocalStore::~LocalStore() {}

QJsonObject LocalStore::readJson(const ObjectKey &key, const QString &fileName, int *costs) const
{
	QFile file(filePath(key, fileName));
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

quint64 LocalStore::count(const QByteArray &typeName) const
{
	QSqlQuery countQuery(_database);
	countQuery.prepare(QStringLiteral("SELECT Count(*) FROM DataIndex WHERE Type = ? AND File IS NOT NULL"));
	countQuery.addBindValue(typeName);
	exec(countQuery, typeName);

	if(countQuery.first())
		return countQuery.value(0).toULongLong();
	else
		return 0;
}

QStringList LocalStore::keys(const QByteArray &typeName) const
{
	QSqlQuery keysQuery(_database);
	keysQuery.prepare(QStringLiteral("SELECT Id FROM DataIndex WHERE Type = ? AND File IS NOT NULL"));
	keysQuery.addBindValue(typeName);
	exec(keysQuery, typeName);

	QStringList resList;
	while(keysQuery.next())
		resList.append(keysQuery.value(0).toString());
	return resList;
}

QList<QJsonObject> LocalStore::loadAll(const QByteArray &typeName) const
{
	//read transaction used to prevent writes while reading json files
	beginReadTransaction(typeName);

	try {
		QSqlQuery loadQuery(_database);
		loadQuery.prepare(QStringLiteral("SELECT Id, File FROM DataIndex WHERE Type = ? AND File IS NOT NULL"));
		loadQuery.addBindValue(typeName);
		exec(loadQuery, typeName);

		QList<ObjectKey> keys;
		QList<QJsonObject> array;
		QList<int> sizes;
		while(loadQuery.next()) {
			int size;
			ObjectKey key {typeName, loadQuery.value(0).toString()};
			auto json = readJson(key, loadQuery.value(1).toString(), &size);
			keys.append(key);
			array.append(json);
			sizes.append(size);
		}

		_emitter->putCached(keys, array, sizes);

		//commit db
		if(!_database->commit())
			throw LocalStoreException(_defaults, typeName, _database->databaseName(), _database->lastError().text());

		return array;
	} catch(...) {
		_database->rollback();
		throw;
	}
}

QJsonObject LocalStore::load(const ObjectKey &key) const
{
	//check if cached
	QJsonObject json;
	if(_emitter->getCached(key, json))
		return json;

	if(!_database->transaction())
		throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());

	try {
		QSqlQuery loadQuery(_database);
		loadQuery.prepare(QStringLiteral("SELECT File FROM DataIndex WHERE Type = ? AND Id = ? AND File IS NOT NULL"));
		loadQuery.addBindValue(key.typeName);
		loadQuery.addBindValue(key.id);
		exec(loadQuery, key);

		if(loadQuery.first()) {
			int size;
			json = readJson(key, loadQuery.value(0).toString(), &size);
			_emitter->putCached(key, json, size);
		} else
			throw NoDataException(_defaults, key);

		//commit db
		if(!_database->commit())
			throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());

		return json;
	} catch(...) {
		_database->rollback();
		throw;
	}
}

void LocalStore::save(const ObjectKey &key, const QJsonObject &data)
{
	beginWriteTransaction(key);

	try {
		//check if the file exists
		QSqlQuery existQuery(_database);
		existQuery.prepare(QStringLiteral("SELECT Version, File FROM DataIndex WHERE Type = ? AND Id = ?"));
		existQuery.addBindValue(key.typeName);
		existQuery.addBindValue(key.id);
		exec(existQuery, key);

		//create the file device to write to
		quint64 version = 1ull;
		bool existing = existQuery.first();
		if(existing)
			version = existQuery.value(0).toULongLong() + 1ull;

		//perform store operation
		auto resFn = storeChangedImpl(_database,
									  key,
									  version,
									  existing ? existQuery.value(1).toString() : QString(),
									  data,
									  true,
									  existing);

		//commit database changes
		if(!_database->commit())
			throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());

		resFn();
	} catch(...) {
		_emitter->dropCached(key);
		_database->rollback();
		throw;
	}
}

bool LocalStore::remove(const ObjectKey &key)
{
	beginWriteTransaction(key);

	try {
		//load data of existing entry
		QSqlQuery loadQuery(_database);
		loadQuery.prepare(QStringLiteral("SELECT Version, File FROM DataIndex WHERE Type = ? AND Id = ? AND File IS NOT NULL"));
		loadQuery.addBindValue(key.typeName);
		loadQuery.addBindValue(key.id);
		exec(loadQuery, key);

		if(loadQuery.first()) { //stored -> remove it
			auto version = loadQuery.value(0).toULongLong() + 1;

			//"remove" from db
			QSqlQuery removeQuery(_database);
			removeQuery.prepare(QStringLiteral("UPDATE DataIndex SET Version = ?, File = NULL, Checksum = NULL, Changed = 1 WHERE Type = ? AND Id = ?"));
			removeQuery.addBindValue(version);
			removeQuery.addBindValue(key.typeName);
			removeQuery.addBindValue(key.id);
			exec(removeQuery, key);

			//delete the file
			QFile rmFile(filePath(key, loadQuery.value(1).toString()));
			if(!rmFile.remove())
				throw LocalStoreException(_defaults, key, rmFile.fileName(), rmFile.errorString());

			//commit db
			if(!_database->commit())
				throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());

			//update cache
			_emitter->dropCached(key);
			//trigger change signals
			_emitter->triggerChange(key, true, true);

			return true;
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

QList<QJsonObject> LocalStore::find(const QByteArray &typeName, const QString &query, DataStore::SearchMode mode) const
{
	auto searchQuery = query;
	if(mode != DataStore::RegexpMode) { //escape any of the like wildcard literals
		if(mode != DataStore::WildcardMode)
			searchQuery.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
		searchQuery.replace(QLatin1Char('%'), QStringLiteral("\\%"));
		searchQuery.replace(QLatin1Char('_'), QStringLiteral("\\_"));
	}

	switch(mode) {
	case DataStore::WildcardMode:
	{
		//replace any unescaped * or ? by % and _
		const QRegularExpression searchRepRegex1(QStringLiteral(R"__(((?<!\\)(?:\\\\)*)\*)__"));
		const QRegularExpression searchRepRegex2(QStringLiteral(R"__(((?<!\\)(?:\\\\)*)\?)__"));
		searchQuery.replace(searchRepRegex1, QStringLiteral("\\1%"));
		searchQuery.replace(searchRepRegex2, QStringLiteral("\\1_"));
		break;
	}
	case DataStore::ContainsMode:
		searchQuery = QLatin1Char('%') + searchQuery + QLatin1Char('%');
		break;
	case DataStore::StartsWithMode:
		searchQuery = searchQuery + QLatin1Char('%');
		break;
	case DataStore::EndsWithMode:
		searchQuery = QLatin1Char('%') + searchQuery;
		break;
	default:
		break;
	}

	beginReadTransaction(typeName);

	try {
		QSqlQuery findQuery(_database);
		auto queryStr = QStringLiteral("SELECT Id, File FROM DataIndex WHERE Type = ? AND %1 AND File IS NOT NULL");
		if(mode == DataStore::RegexpMode)
			queryStr = queryStr.arg(QStringLiteral("Id REGEXP ?"));
		else
			queryStr = queryStr.arg(QStringLiteral("Id LIKE ? ESCAPE '\\'"));
		findQuery.prepare(queryStr);
		findQuery.addBindValue(typeName);
		findQuery.addBindValue(searchQuery);
		exec(findQuery, typeName);

		QList<ObjectKey> keys;
		QList<QJsonObject> array;
		QList<int> sizes;
		while(findQuery.next()) {
			int size;
			ObjectKey key {typeName, findQuery.value(0).toString()};
			auto json = readJson(key, findQuery.value(1).toString(), &size);
			keys.append(key);
			array.append(json);
			sizes.append(size);
		}

		_emitter->putCached(keys, array, sizes);

		if(!_database->commit())
			throw LocalStoreException(_defaults, typeName, _database->databaseName(), _database->lastError().text());

		return array;
	} catch(...) {
		_database->rollback();
		throw;
	}
}

void LocalStore::clear(const QByteArray &typeName)
{
	beginWriteTransaction(typeName, true);

	try {
		QSqlQuery clearQuery(_database);
		clearQuery.prepare(QStringLiteral("UPDATE DataIndex "
										  "SET Version = Version + 1, File = NULL, Checksum = NULL, Changed = 1 "
										  "WHERE Type = ? AND File IS NOT NULL"));
		clearQuery.addBindValue(typeName);
		exec(clearQuery, typeName);

		auto tableDir = typeDirectory(typeName);
		if(!tableDir.removeRecursively()) {
			logWarning() << "Failed to delete cleared data directory for type"
						 << typeName;
		}

		if(!_database->commit())
			throw LocalStoreException(_defaults, typeName, _database->databaseName(), _database->lastError().text());

		//clear cache
		_emitter->dropCached(typeName);
		//trigger change signals
		_emitter->triggerClear(typeName);
	} catch(...) {
		_database->rollback();
		throw;
	}
}

void LocalStore::reset(bool keepData)
{
	beginWriteTransaction(ObjectKey{"any"}, true);

	try {
		if(keepData) { //mark everything changed, to upload if needed
			QSqlQuery resetQuery(_database);
			resetQuery.prepare(QStringLiteral("UPDATE DataIndex SET Changed = 1"));
			exec(resetQuery);

			//also: delete all not done device changes
			QSqlQuery clearDevicesQuery(_database);
			clearDevicesQuery.prepare(QStringLiteral("DELETE FROM DeviceUploads"));
			exec(clearDevicesQuery);
		} else { //delete everything
			QSqlQuery resetQuery(_database);
			resetQuery.prepare(QStringLiteral("DELETE FROM DataIndex"));
			exec(resetQuery);

			//note: resets are local only, so they dont trigger any changecontroller stuff

			auto tableDir = _defaults.storageDir();
			if(tableDir.cd(QStringLiteral("store"))) {
				if(!tableDir.removeRecursively()) //no rollback, as partially removed is possible, better keep junk data...
					logWarning() << "Failed to delete store directory" << tableDir.absolutePath();
			}
		}

		if(!_database->commit())
			throw LocalStoreException(_defaults, QByteArray("<any>"), _database->databaseName(), _database->lastError().text());

		//only if data was actually deleted
		if(!keepData) {
			//clear cache
			_emitter->dropCached();
			//trigger change signals
			_emitter->triggerReset();
		}
	} catch(...) {
		_database->rollback();
		throw;
	}
}

quint32 LocalStore::changeCount() const
{
	QSqlQuery countQuery(_database);
	countQuery.prepare(QStringLiteral("SELECT Sum(rows) FROM ( "
									  "		SELECT Count(*) AS rows FROM DataIndex "
									  "		WHERE Changed = 1"
									  "		UNION ALL"
									  "		SELECT Count(*) AS rows FROM DataIndex "
									  "		INNER JOIN DeviceUploads "
									  "		ON DataIndex.Type = DeviceUploads.Type "
									  "		AND DataIndex.Id = DeviceUploads.Id "
									  "		WHERE NOT (DataIndex.Changed = 1 AND File IS NULL)"
									  ")"));
	exec(countQuery);

	if(countQuery.first())
		return countQuery.value(0).toUInt();
	else
		return 0;
}

void LocalStore::loadChanges(int limit, const function<bool(ObjectKey, quint64, QString, QUuid)> &visitor) const
{
	beginReadTransaction();

	try {
		QSqlQuery readChangesQuery(_database);
		readChangesQuery.prepare(QStringLiteral("SELECT Type, Id, Version, File FROM DataIndex WHERE Changed = 1 LIMIT ?"));
		readChangesQuery.addBindValue(limit);
		exec(readChangesQuery);

		auto cnt = 0;
		auto skip = false;
		while(readChangesQuery.next()) {
			cnt++;
			if(!visitor({readChangesQuery.value(0).toByteArray(), readChangesQuery.value(1).toString()},
						readChangesQuery.value(2).toULongLong(),
						readChangesQuery.value(3).toString(),
						QUuid())) {
				skip = true;
				break;
			}
		}

		if(!skip && cnt < limit) {
			QSqlQuery readDeviceChangesQuery(_database);
			readDeviceChangesQuery.prepare(QStringLiteral("SELECT DeviceUploads.Type, DeviceUploads.Id, DataIndex.Version, DataIndex.File, DeviceUploads.Device "
														  "FROM DeviceUploads "
														  "INNER JOIN DataIndex "
														  "ON (DeviceUploads.Type = DataIndex.Type AND DeviceUploads.Id = DataIndex.Id) "
														  "WHERE NOT (DataIndex.Changed = 1 AND File IS NULL) " //only those that haven't been operated on before
														  "LIMIT ?"));
			readDeviceChangesQuery.addBindValue(limit - cnt);
			exec(readDeviceChangesQuery);

			while(readDeviceChangesQuery.next()) {
				if(!visitor({readDeviceChangesQuery.value(0).toByteArray(), readDeviceChangesQuery.value(1).toString()},
							readDeviceChangesQuery.value(2).toULongLong(),
							readDeviceChangesQuery.value(3).toString(),
							readDeviceChangesQuery.value(4).toUuid())) {
					skip = true;
					break;
				}
			}
		}

		if(!_database->commit())
			throw LocalStoreException(_defaults, QByteArray("<any>"), _database->databaseName(), _database->lastError().text());
	} catch(...) {
		_database->rollback();
		throw;
	}
}

void LocalStore::markUnchanged(const ObjectKey &key, quint64 version, bool isDelete)
{
	markUnchangedImpl(_database, key, version, isDelete);
}

void LocalStore::removeDeviceChange(const ObjectKey &key, const QUuid &deviceId)
{
	QSqlQuery rmDeviceQuery(_database);
	rmDeviceQuery.prepare(QStringLiteral("DELETE FROM DeviceUploads WHERE Type = ? AND Id = ? AND Device = ?"));
	rmDeviceQuery.addBindValue(key.typeName);
	rmDeviceQuery.addBindValue(key.id);
	rmDeviceQuery.addBindValue(deviceId);
	exec(rmDeviceQuery);
}

LocalStore::SyncScope LocalStore::startSync(const ObjectKey &key) const
{
	return SyncScope(_defaults, key, const_cast<LocalStore*>(this));
}

tuple<LocalStore::ChangeType, quint64, QString, QByteArray> LocalStore::loadChangeInfo(SyncScope &scope) const
{
	SCOPE_ASSERT();

	QSqlQuery loadChangeQuery(scope.d->database);
	loadChangeQuery.prepare(QStringLiteral("SELECT Version, File, Checksum FROM DataIndex WHERE Type = ? AND Id = ?"));
	loadChangeQuery.addBindValue(scope.d->key.typeName);
	loadChangeQuery.addBindValue(scope.d->key.id);
	exec(loadChangeQuery);

	if(loadChangeQuery.first()) {
		auto version = loadChangeQuery.value(0).toULongLong();
		auto file = loadChangeQuery.value(1).toString();
		auto checksum = loadChangeQuery.value(2).toByteArray();
		if(file.isNull())
			return make_tuple(ExistsDeleted, version, QString(), QByteArray());
		else
			return make_tuple(Exists, version, file, checksum);
	} else
		return make_tuple(NoExists, 0, QString(), QByteArray());
}

void LocalStore::updateVersion(SyncScope &scope, quint64 oldVersion, quint64 newVersion, bool changed)
{
	SCOPE_ASSERT();
	QSqlQuery updateQuery(scope.d->database);
	updateQuery.prepare(QStringLiteral("UPDATE DataIndex SET Version = ?, Changed = ? WHERE Type = ? AND Id = ? AND Version = ?"));
	updateQuery.addBindValue(newVersion);
	updateQuery.addBindValue(changed);
	updateQuery.addBindValue(scope.d->key.typeName);
	updateQuery.addBindValue(scope.d->key.id);
	updateQuery.addBindValue(oldVersion);
	exec(updateQuery, scope.d->key);

	//notify change controller
	if(changed) {
		Q_ASSERT_X(!scope.d->afterCommit, Q_FUNC_INFO, "Only 1 after commit action can be defined");
		scope.d->afterCommit = [this]() {
			//trigger a change upload
			_emitter->triggerUpload();
		};
	}
}

void LocalStore::storeChanged(SyncScope &scope, quint64 version, const QString &fileName, const QJsonObject &data, bool changed, LocalStore::ChangeType localState)
{
	SCOPE_ASSERT();
	Q_ASSERT_X(!scope.d->afterCommit, Q_FUNC_INFO, "Only 1 after commit action can be defined");
	scope.d->afterCommit = storeChangedImpl(scope.d->database, scope.d->key, version, fileName, data, changed, localState != NoExists);
}

void LocalStore::storeDeleted(SyncScope &scope, quint64 version, bool changed, ChangeType localState)
{
	SCOPE_ASSERT();

	QString fileName;
	bool existing;
	switch (localState) {
	case Exists:
	{
		QSqlQuery loadQuery(scope.d->database);
		loadQuery.prepare(QStringLiteral("SELECT File FROM DataIndex WHERE Type = ? AND Id = ? AND File IS NOT NULL"));
		loadQuery.addBindValue(scope.d->key.typeName);
		loadQuery.addBindValue(scope.d->key.id);
		exec(loadQuery, scope.d->key);

		if(loadQuery.first())
			fileName = filePath(scope.d->key, loadQuery.value(0).toString());
		Q_FALLTHROUGH();
	}
	case ExistsDeleted:
		existing = true;
		break;
	case NoExists:
		existing = false;
		break;
	default:
		Q_UNREACHABLE();
		break;
	}

	if(existing) {
		QSqlQuery updateQuery(scope.d->database);
		updateQuery.prepare(QStringLiteral("UPDATE DataIndex SET Version = ?, File = NULL, Checksum = NULL, Changed = ? WHERE Type = ? AND Id = ?"));
		updateQuery.addBindValue(version);
		updateQuery.addBindValue(changed);
		updateQuery.addBindValue(scope.d->key.typeName);
		updateQuery.addBindValue(scope.d->key.id);
		exec(updateQuery, scope.d->key);
	} else {
		QSqlQuery insertQuery(scope.d->database);
		insertQuery.prepare(QStringLiteral("INSERT INTO DataIndex (Type, Id, Version, File, Checksum, Changed) VALUES(?, ?, ?, NULL, NULL, ?)"));
		insertQuery.addBindValue(scope.d->key.typeName);
		insertQuery.addBindValue(scope.d->key.id);
		insertQuery.addBindValue(version);
		insertQuery.addBindValue(changed);
		exec(insertQuery, scope.d->key);
	}

	//delete the file, if one exists
	if(!fileName.isNull()) {
		QFile rmFile(fileName);
		if(!rmFile.remove())
			throw LocalStoreException(_defaults, scope.d->key, rmFile.fileName(), rmFile.errorString());
	}

	Q_ASSERT_X(!scope.d->afterCommit, Q_FUNC_INFO, "Only 1 after commit action can be defined");
	if(localState == Exists) {
		auto key = scope.d->key;
		scope.d->afterCommit = [this, key, changed]() {
			//update cache
			_emitter->dropCached(key);
			//notify others
			_emitter->triggerChange(key, true, changed);
		};
	} else if(changed){
		scope.d->afterCommit = [this]() {
			//trigger a change upload
			_emitter->triggerUpload();
		};
	}
}

void LocalStore::markUnchanged(SyncScope &scope, quint64 oldVersion, bool isDelete)
{
	SCOPE_ASSERT();
	markUnchangedImpl(scope.d->database, scope.d->key, oldVersion, isDelete);
}

void LocalStore::commitSync(SyncScope &scope) const
{
	SCOPE_ASSERT();

	if(!scope.d->database->commit())
		throw LocalStoreException(_defaults, scope.d->key, scope.d->database->databaseName(), scope.d->database->lastError().text());

	if(scope.d->afterCommit)
		scope.d->afterCommit();

	scope.d->database = DatabaseRef(); //clear the ref, so it won't rollback
}

void LocalStore::prepareAccountAdded(const QUuid &deviceId)
{
	try {
		QSqlQuery insertQuery(_database);
		insertQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO DeviceUploads (Type, Id, Device) "
										   "SELECT Type, Id, ? FROM DataIndex"));
		insertQuery.addBindValue(deviceId);
		exec(insertQuery);

		if(insertQuery.numRowsAffected() != 0) //in case of -1 (unknown), simply assue changed
			_emitter->triggerUpload();
	} catch(Exception &e) {
		logCritical() << "Failed to prepare added account with error:" << e.what();
	}
}

QDir LocalStore::typeDirectory(const ObjectKey &key) const
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

QString LocalStore::filePath(const QDir &typeDir, const QString &baseName) const
{
	return typeDir.absoluteFilePath(baseName + QStringLiteral(".dat"));
}

QString LocalStore::filePath(const ObjectKey &key, const QString &baseName) const
{
	return filePath(typeDirectory(key), baseName);
}

void LocalStore::beginReadTransaction(const ObjectKey &key) const
{
	if(!_database->transaction())
		throw LocalStoreException(_defaults, key, _database->databaseName(), _database->lastError().text());
}

void LocalStore::beginWriteTransaction(const ObjectKey &key, bool exclusive)
{
	QSqlQuery transactQuery(_database);
	if(!transactQuery.exec(QStringLiteral("BEGIN %1 TRANSACTION")
						   .arg(exclusive ? QStringLiteral("EXCLUSIVE") : QStringLiteral("IMMEDIATE")))) {
		throw LocalStoreException(_defaults,
								  key,
								  transactQuery.executedQuery().simplified(),
								  transactQuery.lastError().text());
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

function<void()> LocalStore::storeChangedImpl(const DatabaseRef &db, const ObjectKey &key, quint64 version, const QString &fileName, const QJsonObject &data, bool changed, bool existing)
{
	auto tableDir = typeDirectory(key);
	QScopedPointer<QFileDevice> device;
	function<bool(QFileDevice*)> fileCommitFn;

	if(existing && !fileName.isNull()) {
		auto file = new QSaveFile(filePath(tableDir, fileName));
		device.reset(file);
		if(!file->open(QIODevice::WriteOnly))
			throw LocalStoreException(_defaults, key, file->fileName(), file->errorString());
		fileCommitFn = [](QFileDevice *d){
			return static_cast<QSaveFile*>(d)->commit();
		};
	} else {
		auto fileName = QStringLiteral("%1XXXXXX")
						.arg(QString::fromUtf8(QUuid::createUuid().toRfc4122().toHex()));
		auto file = new QTemporaryFile(filePath(tableDir, fileName));
		device.reset(file);
		if(!file->open())
			throw LocalStoreException(_defaults, key, file->fileName(), file->errorString());
		fileCommitFn = [](QFileDevice *d){
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
	device->write(QJsonDocument(data).toBinaryData());
	if(device->error() != QFile::NoError)
		throw LocalStoreException(_defaults, key, device->fileName(), device->errorString());

	//save key in database
	QFileInfo info(device->fileName());
	if(existing) {
		QSqlQuery updateQuery(db);
		updateQuery.prepare(QStringLiteral("UPDATE DataIndex SET Version = ?, File = ?, Checksum = ?, Changed = ? WHERE Type = ? AND Id = ?"));
		updateQuery.addBindValue(version);
		updateQuery.addBindValue(tableDir.relativeFilePath(info.completeBaseName())); //still update file, in case it was set to NULL
		updateQuery.addBindValue(SyncHelper::jsonHash(data));
		updateQuery.addBindValue(changed);
		updateQuery.addBindValue(key.typeName);
		updateQuery.addBindValue(key.id);
		exec(updateQuery, key);
	} else {
		QSqlQuery insertQuery(db);
		insertQuery.prepare(QStringLiteral("INSERT INTO DataIndex (Type, Id, Version, File, Checksum, Changed) VALUES(?, ?, ?, ?, ?, ?)"));
		insertQuery.addBindValue(key.typeName);
		insertQuery.addBindValue(key.id);
		insertQuery.addBindValue(version);
		insertQuery.addBindValue(tableDir.relativeFilePath(info.completeBaseName()));
		insertQuery.addBindValue(SyncHelper::jsonHash(data));
		insertQuery.addBindValue(changed);
		exec(insertQuery, key);
	}

	//complete the file-save (last before commit!)
	if(!fileCommitFn(device.data()))
		throw LocalStoreException(_defaults, key, device->fileName(), device->errorString());

	//update cache
	_emitter->putCached(key, data, info.size());

	return [this, key, changed]() {
		//trigger change signals
		_emitter->triggerChange(key, false, changed);
	};
}

void LocalStore::markUnchangedImpl(const DatabaseRef &db, const ObjectKey &key, quint64 version, bool isDelete)
{
	QSqlQuery completeQuery(db);
	if(isDelete && !_defaults.property(Defaults::PersistDeleted).toBool())
		completeQuery.prepare(QStringLiteral("DELETE FROM DataIndex WHERE Type = ? AND Id = ? AND Version = ? AND File IS NULL"));
	else
		completeQuery.prepare(QStringLiteral("UPDATE DataIndex SET Changed = 0 WHERE Type = ? AND Id = ? AND Version = ?"));
	completeQuery.addBindValue(key.typeName);
	completeQuery.addBindValue(key.id);
	completeQuery.addBindValue(version);
	exec(completeQuery);
}

// ------------- SyncScope -------------

LocalStore::SyncScope::SyncScope(const Defaults &defaults, const ObjectKey &key, LocalStore *owner) :
	d(new Private(defaults, key, owner))
{
	QSqlQuery transactQuery(d->database);
	if(!transactQuery.exec(QStringLiteral("BEGIN IMMEDIATE TRANSACTION"))) {
		throw LocalStoreException(defaults,
								  key,
								  transactQuery.executedQuery().simplified(),
								  transactQuery.lastError().text());
	}
}

LocalStore::SyncScope::SyncScope(LocalStore::SyncScope &&other) :
	d()
{
	d.swap(other.d);
}

LocalStore::SyncScope::~SyncScope()
{
	if(d->database.isValid())
		d->database->rollback();
}



LocalStore::SyncScope::Private::Private(const Defaults &defaults, const ObjectKey &key, LocalStore *owner) :
	key(key),
	database(defaults.aquireDatabase(owner)),
	afterCommit()
{}
