#include "databasewatcher_p.h"

#include <QtSql/QSqlRecord>
#include <QtSql/QSqlIndex>

using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logDbWatcher, "qt.datasync.DatabaseWatcher")

const QString DatabaseWatcher::MetaTable = QStringLiteral("__qtdatasync_meta_data");
const QString DatabaseWatcher::TablePrefix = QStringLiteral("__qtdatasync_sync_data_");

DatabaseWatcher::DatabaseWatcher(QSqlDatabase &&db, QObject *parent) :
	QObject{parent},
	_db{std::move(db)}
{
	auto driver = _db.driver();
	connect(driver, qOverload<const QString &>(&QSqlDriver::notification),
			this, &DatabaseWatcher::dbNotify);
}

QSqlDatabase DatabaseWatcher::database() const
{
	return _db;
}

bool DatabaseWatcher::hasTables() const
{
	return !_tables.isEmpty();
}

QStringList DatabaseWatcher::tables() const
{
	return _tables.keys();
}

bool DatabaseWatcher::hasTable(const QString &name) const
{
	return _tables.contains(name);
}

void DatabaseWatcher::reactivateTables(bool liveSync)
{
	try {
		ExQuery getActiveQuery{_db, ErrorScope::Database, {}};
		getActiveQuery.prepare(QStringLiteral("SELECT tableName "
											  "FROM %1 "
											  "WHERE state == ?;")
							   .arg(MetaTable));
		getActiveQuery.addBindValue(static_cast<int>(TableState::Active));
		getActiveQuery.exec();
		while (getActiveQuery.next())
			addTable(getActiveQuery.value(0).toString(), liveSync);
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		throw TableException {
			{},
			QStringLiteral("Failed to list tables to be reactivated"),
			error.error()
		};
	}
}

void DatabaseWatcher::addAllTables(bool liveSync, QSql::TableType type)
{
	for (const auto &table : _db.tables(type)) {
		if (table.startsWith(QStringLiteral("__qtdatasync_")))
			continue;
		addTable(table, liveSync);
	}
}

void DatabaseWatcher::addTable(const QString &name, bool liveSync, const QStringList &fields, QString primaryType)
{
	const QString syncTableName = TablePrefix + name;

	try {
		// step 0: verify primary key
		const auto pIndex = _db.primaryIndex(name);
		if (pIndex.count() != 1) {
			qCCritical(logDbWatcher) << "Cannot synchronize table" << name
									 << "with composite primary key!";
			throw TableException{name, QStringLiteral("Cannot synchronize table with composite primary key!"), _db.lastError()};
		}
		const auto pKey = pIndex.fieldName(0);
		const auto escPKey = fieldName(pKey);

		// step 1: create the global sync table
		if (!_db.tables().contains(MetaTable)) {
			ExQuery createTableQuery{_db, ErrorScope::Database, name};
			createTableQuery.exec(QStringLiteral("CREATE TABLE %1 ( "
												 "	tableName	TEXT NOT NULL, "
												 "	pkey		TEXT NOT NULL, "
												 "	state		INTEGER NOT NULL DEFAULT 0 CHECK(state >= 0 AND state <= 2) , "
												 "	lastSync	TEXT, "
												 "	PRIMARY KEY(tableName) "
												 ") WITHOUT ROWID;")
									  .arg(MetaTable));
			qCInfo(logDbWatcher) << "Created sync metadata table";
		} else
			qCDebug(logDbWatcher) << "Sync metadata table already exist";

		// step 2: update internal tables
		ExTransaction transact{_db, name};
		// step 2.1: create the sync attachment table, if not existant yet
		const auto escName = tableName(name);
		const auto escSyncTableName = tableName(syncTableName);
		if (!_db.tables().contains(syncTableName)) {
			if (primaryType.isEmpty())
				primaryType = sqlTypeName(_db.record(name).field(pKey));

			ExQuery createTableQuery{_db, ErrorScope::Table, name};
			createTableQuery.exec(QStringLiteral("CREATE TABLE %1 ( "
												 "	pkey	%2 NOT NULL, "
												 "	tstamp	TEXT NOT NULL, "
												 "	changed	INTEGER NOT NULL CHECK(changed >= 0 AND changed <= 2), "  // (unchanged, changed, deleted)
												 "	PRIMARY KEY(pkey)"
												 ");")
									  .arg(escSyncTableName, primaryType));
			qCDebug(logDbWatcher) << "Created sync table for" << name;

			ExQuery createIndexQuery{_db, ErrorScope::Table, name};
			createIndexQuery.exec(QStringLiteral("CREATE INDEX %1 ON %2 (changed ASC);")
									  .arg(tableName(syncTableName + QStringLiteral("_idx_changed")),
										   escSyncTableName));
			qCDebug(logDbWatcher) << "Created sync table changed index for" << name;

			ExQuery createInsertTrigger{_db, ErrorScope::Table, name};
			createInsertTrigger.exec(QStringLiteral("CREATE TRIGGER %1 "
													"AFTER INSERT ON %3 "
													"FOR EACH ROW "
													"BEGIN "
														// add or replace, in case deleted still exists
													"	INSERT OR REPLACE INTO %2 "
													"	(pkey, tstamp, changed) "
													"	VALUES(NEW.%4, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), 1); "  // 1 == changed
													"END;")
										 .arg(tableName(syncTableName + QStringLiteral("_insert_trigger")),
											  escSyncTableName,
											  escName,
											  escPKey));
			qCDebug(logDbWatcher) << "Created sync table insert trigger index for" << name;

			ExQuery createUpdateTrigger{_db, ErrorScope::Table, name};
			createUpdateTrigger.exec(QStringLiteral("CREATE TRIGGER %1 "
													"AFTER UPDATE ON %3 "
													"FOR EACH ROW "
													"WHEN NEW.%4 == OLD.%4 "
													"BEGIN "
														// set version and changed
													"	UPDATE %2 "
													"	SET tstamp = strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), changed = 1 "  // 1 == changed
													"	WHERE pkey = NEW.%4; "
													"END;")
										 .arg(tableName(syncTableName + QStringLiteral("_update_trigger")),
											  escSyncTableName,
											  escName,
											  escPKey));
			qCDebug(logDbWatcher) << "Created sync table update trigger index for" << name;

			// TODO not good like that, somehow link old/new and upload old with new data
			ExQuery createRenameTrigger{_db, ErrorScope::Table, name};
			createRenameTrigger.exec(QStringLiteral("CREATE TRIGGER %1 "
													"AFTER UPDATE ON %3 "
													"FOR EACH ROW "
													"WHEN NEW.%4 != OLD.%4 "
													"BEGIN "
														// mark old id as deleted
													"	UPDATE %2 "
													"	SET tstamp = strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), changed = 1 "  // 1 == changed
													"	WHERE pkey = OLD.%4; "
														// then insert (or replace) new entry with new id but same timestamp
													"	INSERT OR REPLACE INTO %2 "
													"	(pkey, tstamp, changed) "
													"	VALUES(NEW.%4, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), 1); "  // 1 == changed
													"END;")
										 .arg(tableName(syncTableName + QStringLiteral("_rename_trigger")),
											  escSyncTableName,
											  escName,
											  escPKey));
			qCDebug(logDbWatcher) << "Created sync table rename trigger index for" << name;

			ExQuery createDeleteTrigger{_db, ErrorScope::Table, name};
			createDeleteTrigger.exec(QStringLiteral("CREATE TRIGGER %1 "
													"AFTER DELETE ON %3 "
													"FOR EACH ROW "
													"BEGIN "
														// set version and changed
													"	UPDATE %2 "
													"	SET tstamp = strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), changed = 1 "  // 1 == changed
													"	WHERE pkey = OLD.%4; "
													"END;")
										 .arg(tableName(syncTableName + QStringLiteral("_delete_trigger")),
											  escSyncTableName,
											  escName,
											  escPKey));
			qCDebug(logDbWatcher) << "Created sync table delete trigger index for" << name;

			ExQuery inflateTableQuery{_db, ErrorScope::Table, name};
			inflateTableQuery.exec(QStringLiteral("INSERT INTO %1 "
												  "(pkey, tstamp, changed) "
												  "SELECT %3, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), 1 "
												  "FROM %2;")
									   .arg(escSyncTableName,
											escName,
											escPKey));
			qCDebug(logDbWatcher) << "Inflated sync table for" << name
								  << "with current data of that table";

			qCInfo(logDbWatcher) << "Created synchronization tables for table" << name;
		} else
			qCDebug(logDbWatcher) << "Sync tables for table" << name << "already exist";

		// step 2.2: add to metadata table
		ExQuery addMetaDataQuery{_db, ErrorScope::Table, name};
		addMetaDataQuery.prepare(QStringLiteral("INSERT INTO %1 "
												"(tableName, pkey, state, lastSync) "
												"VALUES(?, ?, ?, NULL) "
												"ON CONFLICT(tableName) DO UPDATE "
												"SET state = excluded.state "
												"WHERE tableName = excluded.tableName;")
								 .arg(MetaTable));
		addMetaDataQuery.addBindValue(name);
		addMetaDataQuery.addBindValue(pKey);
		addMetaDataQuery.addBindValue(static_cast<int>(TableState::Active));
		addMetaDataQuery.exec();
		qCInfo(logDbWatcher) << "Enabled synchronization for table" << name;

		// step 2.3: commit transaction
		transact.commit();

		// step 3: register notification hook for the table
		_tables.insert(name, fields);
		if (!_db.driver()->subscribeToNotification(name))
			qCWarning(logDbWatcher) << "Unable to register update notification hook for table" << name;  // no hard error, as data is still synced, just not live

		// step 4: emit signals
		Q_EMIT tableAdded(name, liveSync);
		Q_EMIT triggerSync(name);
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		throw TableException {
			name,
			QStringLiteral("Failed to synchronize table with database error"),
			error.error()
		};
	}
}

void DatabaseWatcher::dropAllTables()
{
	for (auto it = _tables.keyBegin(); it != _tables.keyEnd(); ++it)
		dropTable(*it, false);
	_tables.clear();
}

void DatabaseWatcher::dropTable(const QString &name, bool removeRef)
{
	_db.driver()->unsubscribeFromNotification(name);
	if (removeRef)
		_tables.remove(name);
	Q_EMIT tableRemoved(name);
}

void DatabaseWatcher::removeAllTables()
{
	for (auto it = _tables.keyBegin(); it != _tables.keyEnd(); ++it)
		removeTable(*it, false);
	_tables.clear();
}

void DatabaseWatcher::removeTable(const QString &name, bool removeRef)
{
	try {
		ExQuery removeMetaData{_db, ErrorScope::Database, name};
		removeMetaData.prepare(QStringLiteral("UPDATE %1 "
											  "SET state = ? "
											  "WHERE tableName == ?;")
								   .arg(MetaTable));
		removeMetaData.addBindValue(static_cast<int>(TableState::Inactive));
		removeMetaData.addBindValue(name);
		removeMetaData.exec();
		qCInfo(logDbWatcher) << "Disabled synchronization for" << name;
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		throw TableException {
			name,
			QStringLiteral("Failed to deactivate synchronization for table with database error"),
			error.error()
		};
	}

	dropTable(name, removeRef);
}

void DatabaseWatcher::unsyncAllTables()
{
	for (auto it = _tables.keyBegin(); it != _tables.keyEnd(); ++it)
		unsyncTable(*it, false);
	_tables.clear();

	try {
		ExTransaction transact{_db, {}};

		// step 1: check if metadata table is empty
		ExQuery checkMetaQuery{_db, ErrorScope::Database, {}};
		checkMetaQuery.exec(QStringLiteral("SELECT COUNT(*) FROM %1;")
								.arg(MetaTable));

		// step 2: if empty -> drop the table
		if (checkMetaQuery.first() && checkMetaQuery.value(0) == 0) {
			checkMetaQuery.finish();
			ExQuery dropMetaTable{_db, ErrorScope::Database, {}};
			dropMetaTable.exec(QStringLiteral("DROP TABLE %1;").arg(MetaTable));
			qCInfo(logDbWatcher) << "Dropped all synchronization data";
		}

		// step 3: commit
		_pKeyCache.clear();
		transact.commit();
		qCDebug(logDbWatcher) << "Removed sync metadata table";
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		throw TableException {
			{},
			QStringLiteral("Failed clear metadata with database error"),
			error.error()
		};
	}
}

void DatabaseWatcher::unsyncTable(const QString &name, bool removeRef)
{
	try {
		// step 1: remove table
		removeTable(name, removeRef);

		// step 2: remove all sync stuff
		ExTransaction transact{_db, name};

		// step 2.1 remove sync metadata
		ExQuery removeMetaData{_db, ErrorScope::Database, name};
		removeMetaData.prepare(QStringLiteral("DELETE FROM %1 "
											  "WHERE tableName == ?;")
								   .arg(MetaTable));
		removeMetaData.addBindValue(name);
		removeMetaData.exec();
		_pKeyCache.remove(name);
		qCDebug(logDbWatcher) << "Removed metadata for" << name;

		// step 2.2 remove specific sync tables
		const QString syncTableName = TablePrefix + name;
		if (_db.tables().contains(syncTableName)) {
			ExQuery dropDeleteTrigger{_db, ErrorScope::Table, name};
			dropDeleteTrigger.exec(QStringLiteral("DROP TRIGGER %1;").arg(tableName(syncTableName + QStringLiteral("_delete_trigger"))));
			qCDebug(logDbWatcher) << "Dropped sync table delete trigger index for" << name;

			ExQuery dropRenameTrigger{_db, ErrorScope::Table, name};
			dropRenameTrigger.exec(QStringLiteral("DROP TRIGGER %1;").arg(tableName(syncTableName + QStringLiteral("_rename_trigger"))));
			qCDebug(logDbWatcher) << "Dropped sync table rename trigger index for" << name;

			ExQuery dropUpdateTrigger{_db, ErrorScope::Table, name};
			dropUpdateTrigger.exec(QStringLiteral("DROP TRIGGER %1;").arg(tableName(syncTableName + QStringLiteral("_update_trigger"))));
			qCDebug(logDbWatcher) << "Dropped sync table update trigger index for" << name;

			ExQuery dropInsertTrigger{_db, ErrorScope::Table, name};
			dropInsertTrigger.exec(QStringLiteral("DROP TRIGGER %1;").arg(tableName(syncTableName + QStringLiteral("_insert_trigger"))));
			qCDebug(logDbWatcher) << "Dropped sync table insert trigger index for" << name;

			ExQuery dropIndexQuery{_db, ErrorScope::Table, name};
			dropIndexQuery.exec(QStringLiteral("DROP INDEX %1;").arg(tableName(syncTableName + QStringLiteral("_idx_changed"))));
			qCDebug(logDbWatcher) << "Dropped sync table index for" << name;

			ExQuery dropTableQuery{_db, ErrorScope::Table, name};
			dropTableQuery.exec(QStringLiteral("DROP TABLE %1;").arg(tableName(syncTableName)));
			qCDebug(logDbWatcher) << "Dropped sync table for" << name;
		} else
			qCDebug(logDbWatcher) << "Sync tables for table" << name << "do not exist";

		// step 2.3 commit transaction
		transact.commit();
		qCInfo(logDbWatcher) << "Dropped synchronization data for table" << name;
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		throw TableException {
			name,
			QStringLiteral("Failed to unsynchronize table with database error"),
			error.error()
		};
	}
}

void DatabaseWatcher::resyncAllTables(Engine::ResyncMode direction)
{
	for (auto it = _tables.keyBegin(); it != _tables.keyEnd(); ++it)
		resyncTable(*it, direction);
}

void DatabaseWatcher::resyncTable(const QString &name, Engine::ResyncMode direction)
{
	try {
		ExTransaction transact{_db, name};

		const auto escName = tableName(name);
		const auto escSyncName = tableName(name, true);
		const auto pKey = getPKey(name);

		if (direction.testFlag(Engine::ResyncFlag::ClearLocalData)) {
			ExQuery clearLocalQuery{_db, ErrorScope::Table, name};
			clearLocalQuery.exec(QStringLiteral("DELETE FROM %1;").arg(escName));

			ExQuery cleanLocalQuery{_db, ErrorScope::Table, name};
			cleanLocalQuery.exec(QStringLiteral("DELETE FROM %1;").arg(escSyncName));
		} else if (direction.testFlag(Engine::ResyncFlag::CleanLocalData)) {
			ExQuery cleanLocalQuery{_db, ErrorScope::Table, name};
			cleanLocalQuery.exec(QStringLiteral("DELETE FROM %1;").arg(escSyncName));

			ExQuery inflateTableTrigger{_db, ErrorScope::Table, name};
			inflateTableTrigger.exec(QStringLiteral("INSERT INTO %1 "
													"(pkey, tstamp, changed) "
													"SELECT %3, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), 1 "
													"FROM %2;")
										 .arg(escSyncName,
											  escName,
											  pKey));
		} else {
			if (direction.testFlag(Engine::ResyncFlag::Upload)) {
				ExQuery markAllChangedQuery{_db, ErrorScope::Table, name};
				markAllChangedQuery.prepare(QStringLiteral("UPDATE %1 "
														   "SET changed = ?;")
												.arg(escSyncName));
				markAllChangedQuery.addBindValue(static_cast<int>(ChangeState::Changed));
				markAllChangedQuery.exec();
			}

			if (direction.testFlag(Engine::ResyncFlag::CheckLocalData)) {
				ExQuery inflateTableTrigger{_db, ErrorScope::Table, name};
				inflateTableTrigger.exec(QStringLiteral("INSERT OR IGNORE "
														"INTO %1 "
														"(pkey, tstamp, changed) "
														"SELECT %3, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), 1 "
														"FROM %2;")
											 .arg(escSyncName,
												  escName,
												  pKey));
			}
		}

		if (direction.testFlag(Engine::ResyncFlag::Download)) {
			ExQuery resetLastSyncQuery{_db, ErrorScope::Database, name};
			resetLastSyncQuery.prepare(QStringLiteral("UPDATE %1 "
													  "SET lastSync = NULL "
													  "WHERE tableName == ?;")
										   .arg(MetaTable));
			resetLastSyncQuery.addBindValue(name);
			resetLastSyncQuery.exec();
		}

		transact.commit();

		// emit signals
		if (direction.testFlag(Engine::ResyncFlag::ClearServerData))
			Q_EMIT triggerResync(name, true);
		else if (direction.testFlag(Engine::ResyncFlag::Download))
			Q_EMIT triggerResync(name, false);
		else if (direction.testFlag(Engine::ResyncFlag::Upload) ||
				 direction.testFlag(Engine::ResyncFlag::CheckLocalData))
			Q_EMIT triggerSync(name);
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		throw TableException {
			name,
			QStringLiteral("Failed to re-synchronize table with database error"),
			error.error()
		};
	}
}

std::optional<QDateTime> DatabaseWatcher::lastSync(const QString &tableName)
{
	try {
		ExQuery lastSyncQuery{_db, ErrorScope::Database, tableName};
		lastSyncQuery.prepare(QStringLiteral("SELECT lastSync "
											 "FROM %1 "
											 "WHERE tableName == ?;")
								  .arg(MetaTable));
		lastSyncQuery.addBindValue(tableName);
		lastSyncQuery.exec();

		if (lastSyncQuery.next())
			return lastSyncQuery.value(0).toDateTime().toUTC();
		else
			return QDateTime{}.toUTC();
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		error.emitFor(this);
		return std::nullopt;
	}
}

void DatabaseWatcher::storeData(const LocalData &data)
{
	const auto key = data.key();
	try {
		ExTransaction transact{_db, key};

		// retrieve the primary key
		const auto pKey = getPKey(key.typeName);
		const auto escTableName = tableName(key.typeName);
		const auto escSyncName = tableName(key.typeName, true);

		// check if newer
		ExQuery checkNewerQuery{_db, ErrorScope::Table, key.typeName};
		checkNewerQuery.prepare(QStringLiteral("SELECT tstamp "
											   "FROM %1 "
											   "WHERE pkey = ?;")
									.arg(escSyncName));
		checkNewerQuery.addBindValue(key.id);
		checkNewerQuery.exec();
		if (checkNewerQuery.first()) {
			const auto localMod = checkNewerQuery.value(0).toDateTime();
			if (localMod > data.modified()) {
				qCDebug(logDbWatcher) << "Data with id" << key
									  << "was modified in the cloud, but is newer locally."
									  << "Local date:" << localMod
									  << "Cloud date:" << data.modified();
				updateLastSync(key.typeName, data.uploaded());
				transact.commit();
				return;
			}
		} else
			qCDebug(logDbWatcher) << "No local data for id" << key;

		// update the actual data
		if (const auto value = data.data(); value) {
			// try to insert as a new value
			QStringList insertKeys;
			QStringList updateKeys;
			insertKeys.reserve(value->size());
			updateKeys.reserve(value->size());
			for (auto it = value->keyBegin(), end = value->keyEnd(); it != end; ++it) {
				const auto fName = fieldName(*it);
				insertKeys.append(fName);
				updateKeys.append(QStringLiteral("%1 = excluded.%1").arg(fName));
			}
			const auto paramPlcs = QVector<QString>{
				value->size(),
				QString{QLatin1Char('?')}
			}.toList();

			// TODO okay so? can fail if other unique statements conflict
			ExQuery upsertQuery{_db, ErrorScope::Entry, key};
			upsertQuery.prepare(QStringLiteral("INSERT INTO %1 "
											   "(%2) "
											   "VALUES(%3) "
											   "ON CONFLICT(%4) DO UPDATE "
											   "SET %5 "
											   "WHERE %4 = excluded.%4;")
									.arg(escTableName,
										 insertKeys.join(QStringLiteral(", ")),
										 paramPlcs.join(QStringLiteral(", ")),
										 pKey,
										 updateKeys.join(QStringLiteral(", "))));
			for (const auto &val : *value)
				upsertQuery.addBindValue(val);
			upsertQuery.exec();
			qCDebug(logDbWatcher) << "Updated local data from cloud data for id" << key;
		} else {
			// delete -> delete it
			ExQuery deleteQuery{_db, ErrorScope::Entry, key};
			deleteQuery.prepare(QStringLiteral("DELETE FROM %1 "
											   "WHERE %2 == ?;")
									.arg(escTableName, pKey));
			deleteQuery.addBindValue(key.id);
			deleteQuery.exec();
			qCDebug(logDbWatcher) << "Removed local data for id" << key;
		}
		// update modified, mark unchanged -> now in sync with remote
		markUnchanged(key, data.modified());

		// update last sync
		updateLastSync(key.typeName, data.uploaded());
		transact.commit();
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		if (error.scope() == ErrorScope::Entry)
			markCorrupted(key, data.modified());
		error.emitFor(this);
	}
}

std::optional<LocalData> DatabaseWatcher::loadData(const QString &name)
{
	try {
		ExTransaction transact{_db, name};

		const auto pKey = getPKey(name);

		ExQuery nextDataQuery{_db, ErrorScope::Table, name};
		nextDataQuery.prepare(QStringLiteral("SELECT syncTable.pkey, syncTable.tstamp, dataTable.* "
											 "FROM %1 AS syncTable "
											 "LEFT JOIN %2 AS dataTable "
											 "ON syncTable.pkey == dataTable.%3 "
											 "WHERE syncTable.changed == ? "
											 "ORDER BY syncTable.tstamp ASC "
											 "LIMIT 1")
								  .arg(tableName(name, true),
									   tableName(name),
									   pKey));
		nextDataQuery.addBindValue(static_cast<int>(ChangeState::Changed));
		nextDataQuery.exec();

		if (nextDataQuery.first()) {
			LocalData data;
			data.setKey({name, nextDataQuery.value(0).toString()});
			data.setModified(nextDataQuery.value(1).toDateTime());
			if (const auto fData = nextDataQuery.value(2); !fData.isNull()) {
				const auto record = nextDataQuery.record();
				QVariantHash tableData;
				for (auto i = 2, max = record.count(); i < max; ++i)
					tableData.insert(record.fieldName(i), nextDataQuery.value(i));
				data.setData(tableData);
			}
			return data;
		} else
			return std::nullopt;
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		error.emitFor(this);
		return std::nullopt;
	}
}

void DatabaseWatcher::markUnchanged(const ObjectKey &key, const QDateTime &modified)
{
	try {
		ExQuery markUnchangedQuery{_db, ErrorScope::Table, key.typeName};
		markUnchangedQuery.prepare(QStringLiteral("UPDATE %1 "
												  "SET changed = ?, tstamp = ? "
												  "WHERE pkey = ?;")
									   .arg(tableName(key.typeName, true)));
		markUnchangedQuery.addBindValue(static_cast<int>(ChangeState::Unchanged));
		markUnchangedQuery.addBindValue(modified.toUTC());
		markUnchangedQuery.addBindValue(key.id);
		markUnchangedQuery.exec();
		qCDebug(logDbWatcher) << "Updated metadata for id" << key;
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		error.emitFor(this);
	}
}

void DatabaseWatcher::markCorrupted(const ObjectKey &key, const QDateTime &modified)
{
	try {
		ExQuery insertCorruptedQuery{_db, ErrorScope::Table, key.typeName};
		insertCorruptedQuery.prepare(QStringLiteral("INSERT INTO %1 "
													"(pkey, tstamp, changed) "
													"VALUES(?, ?, ?) "
													"ON CONFLICT(pkey) DO UPDATE "
													"SET changed = excluded.changed "
													"WHERE pkey = excluded.pkey;")
										 .arg(tableName(key.typeName, true)));
		insertCorruptedQuery.addBindValue(key.id);
		insertCorruptedQuery.addBindValue(modified.toUTC());
		insertCorruptedQuery.addBindValue(static_cast<int>(ChangeState::Corrupted));
		insertCorruptedQuery.exec();
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		// do not emit, as it is done by the caller!
	}
}

void DatabaseWatcher::dbNotify(const QString &name)
{
	if (_tables.contains(name))
		Q_EMIT triggerSync(name);
}

QString DatabaseWatcher::sqlTypeName(const QSqlField &field) const
{
	switch (static_cast<int>(field.type())) {
	case QMetaType::UnknownType:
	case QMetaType::Nullptr:
		return QStringLiteral("NULL");
	case QMetaType::Bool:
	case QMetaType::Char:
	case QMetaType::SChar:
	case QMetaType::UChar:
	case QMetaType::Short:
	case QMetaType::UShort:
	case QMetaType::Int:
	case QMetaType::UInt:
	case QMetaType::Long:
	case QMetaType::ULong:
	case QMetaType::LongLong:
	case QMetaType::ULongLong:
		return QStringLiteral("INTEGER");
	case QMetaType::Double:
	case QMetaType::Float:
		return QStringLiteral("REAL");
	case QMetaType::QString:
		return QStringLiteral("TEXT");
	case QMetaType::QByteArray:
	default:
		return QStringLiteral("BLOB");
	}
}

QString DatabaseWatcher::tableName(const QString &table, bool asSyncTable) const
{
	return _db.driver()->escapeIdentifier(asSyncTable ? (TablePrefix + table) : table, QSqlDriver::TableName);
}

QString DatabaseWatcher::fieldName(const QString &field) const
{
	return _db.driver()->escapeIdentifier(field, QSqlDriver::FieldName);
}

QString DatabaseWatcher::getPKey(const QString &table)
{
	if (const auto it = qAsConst(_pKeyCache).find(table);
		it != _pKeyCache.constEnd())
		return *it;

	ExQuery getPKeyQuery{_db, ErrorScope::Database, table};
	getPKeyQuery.prepare(QStringLiteral("SELECT pkey "
										"FROM %1 "
										"WHERE tableName = ?;")
							 .arg(MetaTable));
	getPKeyQuery.addBindValue(table);
	getPKeyQuery.exec();
	if (!getPKeyQuery.first())
		throw SqlException{ErrorScope::Database, table, {}};

	const auto fName = fieldName(getPKeyQuery.value(0).toString());
	_pKeyCache.insert(table, fName);
	return fName;
}

void DatabaseWatcher::updateLastSync(const QString &table, const QDateTime &uploaded)
{
	// do nothing if invalid timestamp
	if (!uploaded.isValid())
		return;
	// update last sync
	ExQuery updateMetaQuery{_db, ErrorScope::Database, table};
	updateMetaQuery.prepare(QStringLiteral("UPDATE %1 "
										   "SET lastSync = ? "
										   "WHERE tableName = ?;")
								.arg(MetaTable));
	updateMetaQuery.addBindValue(uploaded.toUTC());
	updateMetaQuery.addBindValue(table);
	updateMetaQuery.exec();
}



SqlException::SqlException(SqlException::ErrorScope scope, QVariant key, QSqlError error) :
	_scope{scope},
	_key{std::move(key)},
	_error{std::move(error)}
{}

SqlException::ErrorScope SqlException::scope() const
{
	return _scope;
}

QVariant SqlException::key() const
{
	return _key;
}

QSqlError SqlException::error() const
{
	return _error;
}

QString SqlException::message() const
{
	switch (_scope) {
	case ErrorScope::Entry:
		return DatabaseWatcher::tr("Failed to modify local data %1!")
			.arg(_key.value<ObjectKey>().toString());
	case ErrorScope::Table:
		return DatabaseWatcher::tr("Corrupted metadata table for %1 - "
								   "try to re-synchronize it!")
			.arg(_key.toString());
	case ErrorScope::Database:
		return DatabaseWatcher::tr("Corrupted synchronization system table%1. "
								   "Try to re-synchronize all tables!")
			.arg(_key.isNull() ?
					QString{} :
					DatabaseWatcher::tr(" (Accessing for %1)").arg(_key.toString()));
	case ErrorScope::Transaction:
		return DatabaseWatcher::tr("Temporary Database error. "
								   "You can ignore this error, unless it shows up repeatedly!");
	default:
		Q_UNREACHABLE();
	}
}

void SqlException::raise() const
{
	throw *this;
}

ExceptionBase *SqlException::clone() const
{
	return new SqlException{*this};
}

QString SqlException::qWhat() const
{
	return QStringLiteral("SQL-Error with scope %1: %2")
		.arg(QString::fromUtf8(QMetaEnum::fromType<ErrorScope>()
								   .valueToKey(static_cast<int>(_scope))),
			 _error.text());
}

void SqlException::emitFor(DatabaseWatcher *watcher) const
{
	Q_EMIT watcher->databaseError(_scope,
								  message(),
								  _key,
								  _error);
}



ExQuery::ExQuery(QSqlDatabase db, ErrorScope scope, QVariant key) :
	QSqlQuery{db},
	_scope{scope},
	_key{std::move(key)}
{}

void ExQuery::prepare(const QString &query)
{
	if (!QSqlQuery::prepare(query))
		throw SqlException{_scope, std::move(_key), lastError()};
}

void ExQuery::exec()
{
	if (!QSqlQuery::exec())
		throw SqlException{_scope, std::move(_key), lastError()};
}

void ExQuery::exec(const QString &query)
{
	if (!QSqlQuery::exec(query))
		throw SqlException{_scope, std::move(_key), lastError()};
}

ExTransaction::ExTransaction() = default;

ExTransaction::ExTransaction(QSqlDatabase db, QVariant key) :
	_db{db},
	_key{std::move(key)}
{
	if (!_db->transaction())
		throw SqlException{ErrorScope::Transaction, _key, _db->lastError()};
}

ExTransaction::ExTransaction(ExTransaction &&other) noexcept :
	_db{std::move(other._db)}
{}

ExTransaction &ExTransaction::operator=(ExTransaction &&other) noexcept
{
	std::swap(_db, other._db);
	return *this;
}

ExTransaction::~ExTransaction()
{
	if (_db)
		_db->rollback();
}

void ExTransaction::commit()
{
	if (!_db->commit())
		throw SqlException{ErrorScope::Transaction, _key, _db->lastError()};
	_db.reset();
}

void ExTransaction::rollback()
{
	_db->rollback();
	_db.reset();
}
