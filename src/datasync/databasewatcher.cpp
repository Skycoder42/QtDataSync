#include "databasewatcher_p.h"
#include <QtCore/QMetaEnum>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlIndex>

using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logDbWatcher, "qt.datasync.DatabaseWatcher")

const QString DatabaseWatcher::MetaTable = QStringLiteral("__qtdatasync_meta_data");
const QString DatabaseWatcher::FieldTable = QStringLiteral("__qtdatasync_sync_fields");
const QString DatabaseWatcher::ReferenceTable = QStringLiteral("__qtdatasync_sync_references");
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
		while (getActiveQuery.next()) {
			TableConfig config{getActiveQuery.value(0).toString(), _db};
			config.setLiveSyncEnabled(liveSync);
			addTable(config);
		}
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
		TableConfig config{table, _db};
		config.setLiveSyncEnabled(liveSync);
		addTable(config);
	}
}

void DatabaseWatcher::addTable(const TableConfig &config)
{
	const auto name = config.table();
	const QString syncTableName = TablePrefix + name;

	try {
		// step 1: create the global sync meta table
		createMetaTables(name);

		// step 2: create internal tables
		ExTransaction transact{_db, name};
		const auto [pKeyName, pKeyType] = createSyncTables(config);

		// step 3.1: check if already created
		std::optional<TableInfo> info;
		if (!config.forceCreate()) {
			ExQuery getMetaQuery{_db, ErrorScope::Database, name};
			getMetaQuery.prepare(QStringLiteral("SELECT pkeyName, pkeyType "
												"FROM %1 "
												"WHERE tableName == ?;")
									 .arg(MetaTable));
			getMetaQuery.addBindValue(name);
			getMetaQuery.exec();
			if (getMetaQuery.first()) {
				info = TableInfo{};
				info->pKeyInfo.first = getMetaQuery.value(0).toString();
				info->pKeyInfo.second = getMetaQuery.value(1).toInt();
			}
		}

		// step 3.2: table is not synced yet -> create (or force replace)
		if (!info) {
			qCDebug(logDbWatcher) << "Creating or updating synchronization tables";
			// step 3.2.1: add to metadata table (or update config fields)
			ExQuery addMetaDataQuery{_db, ErrorScope::Database, name};
			addMetaDataQuery.prepare(QStringLiteral("INSERT INTO %1 "
													"(tableName, pkeyName, pkeyType, state, lastSync) "
													"VALUES(?, ?, ?, ?, NULL) "
													"ON CONFLICT(tableName) DO UPDATE "
													"SET pkeyName = excluded.pkeyName, "
													"    pkeyType = excluded.pkeyType, "
													"    state = excluded.state "
													"WHERE tableName == excluded.tableName;")
										 .arg(MetaTable));
			addMetaDataQuery.addBindValue(name);
			addMetaDataQuery.addBindValue(pKeyName);
			addMetaDataQuery.addBindValue(pKeyType);
			addMetaDataQuery.addBindValue(static_cast<int>(TableState::Active));
			addMetaDataQuery.exec();
			qCDebug(logDbWatcher) << "Added sync meta entry for" << name;

			// step 3.2.2: Update field references
			ExQuery clearFieldsQuery{_db, ErrorScope::Database, name};
			clearFieldsQuery.prepare(QStringLiteral("DELETE FROM %1 "
													"WHERE tableName == ?;")
										 .arg(FieldTable));
			clearFieldsQuery.addBindValue(name);
			clearFieldsQuery.exec();

			for (const auto &field : config.fields()) {
				ExQuery addFieldQuery{_db, ErrorScope::Database, name};
				addFieldQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO %1 "
													 "(tableName, syncField) "
													 "VALUES(?, ?);")
										  .arg(FieldTable));
				addFieldQuery.addBindValue(name);
				addFieldQuery.addBindValue(field);
				addFieldQuery.exec();
			}
			qCDebug(logDbWatcher) << "Stored custom table fields for" << name;

			// step 3.2.3: Update foreign key references
			ExQuery clearFKeysQuery{_db, ErrorScope::Database, name};
			clearFKeysQuery.prepare(QStringLiteral("DELETE FROM %1 "
												   "WHERE tableName == ?;")
										.arg(ReferenceTable));
			clearFKeysQuery.addBindValue(name);
			clearFKeysQuery.exec();

			for (const auto &ref : config.references()) {
				ExQuery addFKeyQuery{_db, ErrorScope::Database, name};
				addFKeyQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO %1 "
													"(tableName, fKeyTable, fKeyField) "
													"VALUES(?, ?, ?);")
										 .arg(ReferenceTable));
				addFKeyQuery.addBindValue(name);
				addFKeyQuery.addBindValue(ref.first);
				addFKeyQuery.addBindValue(ref.second);
				addFKeyQuery.exec();
			}
			qCDebug(logDbWatcher) << "Stored custom table fields for" << name;

			// step 3.2.4: commit transaction and add table
			transact.commit();
			// pre-populate caches with initial values
			_tables.insert(name, {
									 std::make_pair(pKeyName, pKeyType),
									 config.fields(),
									 config.references()
								 });
			qCInfo(logDbWatcher) << "Added/Updated and enabled synchronization for table" << name;
		} else { // step 3.2: table is synced -> enable and load config
			qCDebug(logDbWatcher) << "Enabeling synchronization tables";
			// step 3.3.1: just enable synchronisation, don't update fields
			ExQuery enableMetaDataQuery{_db, ErrorScope::Database, name};
			enableMetaDataQuery.prepare(QStringLiteral("UPDATE %1 "
													   "SET state = ? "
													   "WHERE tableName == ?;")
											.arg(MetaTable));
			enableMetaDataQuery.addBindValue(static_cast<int>(TableState::Active));
			enableMetaDataQuery.addBindValue(name);
			enableMetaDataQuery.exec();
			qCDebug(logDbWatcher) << "Activated sync meta entry for" << name;

			// step 3.3.2: get all fields
			ExQuery getFieldsQuery{_db, ErrorScope::Database, name};
			getFieldsQuery.prepare(QStringLiteral("SELECT syncField "
												  "FROM %1 "
												  "WHERE tableName == ?;")
									   .arg(FieldTable));
			getFieldsQuery.addBindValue(name);
			getFieldsQuery.exec();
			while (getFieldsQuery.next())
				info->fields.append(getFieldsQuery.value(0).toString());
			qCDebug(logDbWatcher) << "Loaded sync fields for" << name;

			// step 3.3.3: get all references
			ExQuery getReferencesQuery{_db, ErrorScope::Database, name};
			getReferencesQuery.prepare(QStringLiteral("SELECT fKeyTable, fKeyField "
													  "FROM %1 "
													  "WHERE tableName == ?;")
										   .arg(ReferenceTable));
			getReferencesQuery.addBindValue(name);
			getReferencesQuery.exec();
			while (getReferencesQuery.next()) {
				info->references.append(std::make_pair(getReferencesQuery.value(0).toString(),
													  getReferencesQuery.value(1).toString()));
			}
			qCDebug(logDbWatcher) << "Loaded sync references for" << name;

			// step 3.2.4: commit transaction and add table
			transact.commit();
			// leave caches empty, will be loaded on demand
			_tables.insert(name, *info);
			qCInfo(logDbWatcher) << "Enabled synchronization for table" << name;
		}

		// step 3: register notification hook for the table
		if (!_db.driver()->subscribeToNotification(name))
			qCWarning(logDbWatcher) << "Unable to register update notification hook for table" << name;  // no hard error, as data is still synced, just not live

		// step 4: emit signals
		Q_EMIT tableAdded(name, config.isLiveSyncEnabled());
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

			ExQuery dropReferencesTable{_db, ErrorScope::Database, {}};
			dropReferencesTable.exec(QStringLiteral("DROP TABLE %1;").arg(ReferenceTable));

			ExQuery dropFieldsTable{_db, ErrorScope::Database, {}};
			dropFieldsTable.exec(QStringLiteral("DROP TABLE %1;").arg(FieldTable));

			ExQuery dropMetaTable{_db, ErrorScope::Database, {}};
			dropMetaTable.exec(QStringLiteral("DROP TABLE %1;").arg(MetaTable));
			qCInfo(logDbWatcher) << "Dropped all synchronization data";
		}

		// step 3: commit
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
		const auto [pKey, pKeyType] = _tables[name].pKeyInfo;

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
											  fieldName(pKey)));
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
												  fieldName(pKey)));
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

bool DatabaseWatcher::shouldStore(const ObjectKey &key, const CloudData &data)
{
	try {
		ExTransaction transact{_db, key};
		const auto [pKey, pKeyType] = _tables[key.typeName].pKeyInfo;
		const auto res = shouldStoreImpl({
			key,
			std::nullopt,
			data.modified(),
			data.uploaded()
		}, decodeKey(key.id, pKeyType));
		transact.commit();
		return res;
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		error.emitFor(this);
		return false;
	}
}

void DatabaseWatcher::storeData(const LocalData &data)
{
	const auto key = data.key();
	const auto &tInfo = _tables[key.typeName];
	try {
		ExTransaction transact{_db, key};

		// retrieve the primary key
		const auto [pKey, pKeyType] = tInfo.pKeyInfo;
		const auto escKey = decodeKey(key.id, pKeyType);
		const auto escTableName = tableName(key.typeName);
		const auto escSyncName = tableName(key.typeName, true);

		// check if newer
		if (!shouldStoreImpl(data, escKey)) {
			transact.commit();
			return;
		}

		// update the actual data
		if (const auto value = data.data(); value) {
			// create parent entries for all references
			for (const auto &ref : tInfo.references) {  // (table, field)
				qCDebug(logDbWatcher) << "Ensuring existance of parent entry in table" << ref.first;
				// get old parent state
				ExQuery getParentState{_db, ErrorScope::Table, ref.first};
				getParentState.prepare(QStringLiteral("SELECT tstamp, changed "
													  "FROM %1 "
													  "WHERE pkey == ?")
										   .arg(tableName(ref.first, true)));
				getParentState.addBindValue(escKey);
				getParentState.exec();

				// insert parent if not existant yet
				ExQuery createDefaultParent{_db, ErrorScope::Table, ref.first};
				createDefaultParent.prepare(QStringLiteral("INSERT OR IGNORE INTO %1 "
														   "(%2) "
														   "VALUES(?);")
												.arg(tableName(ref.first),
													 fieldName(ref.second)));
				createDefaultParent.addBindValue(escKey);
				createDefaultParent.exec();

				// restore parent state to before beeing inserted (as parent creates are not synchronized)
				if (createDefaultParent.numRowsAffected() != 0) {
					ExQuery fixParentState{_db, ErrorScope::Table, ref.first};
					if (getParentState.first()) {
						fixParentState.prepare(QStringLiteral("UPDATE %1 "
															  "SET tstamp = ?, changed = ? "
															  "WHERE pkey == ?;")
												   .arg(tableName(ref.first, true)));
						fixParentState.addBindValue(getParentState.value(0));
						fixParentState.addBindValue(getParentState.value(1));
					} else {
						fixParentState.prepare(QStringLiteral("DELETE FROM %1 "
															  "WHERE pkey == ?;")
												   .arg(tableName(ref.first, true)));
					}
					fixParentState.addBindValue(escKey);
					fixParentState.exec();
				}
			}

			// try to insert as a new value
			QStringList insertKeys;
			QStringList updateKeys;
			QVariantList queryData;
			insertKeys.reserve(value->size());
			updateKeys.reserve(value->size());
			queryData.reserve(value->size());
			auto realSize = 0;
			for (auto it = value->begin(), end = value->end(); it != end; ++it) {
				if (tInfo.fields.isEmpty() || tInfo.fields.contains(it.key())) {
					const auto fName = fieldName(it.key());
					insertKeys.append(fName);
					updateKeys.append(QStringLiteral("%1 = excluded.%1").arg(fName));
					queryData.append(*it);
					++realSize;
				}
			}
			const auto paramPlcs = QVector<QString>{
				realSize,
				QString{QLatin1Char('?')}
			}.toList();

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
										 fieldName(pKey),
										 updateKeys.join(QStringLiteral(", "))));
			for (const auto &val : qAsConst(queryData))
				upsertQuery.addBindValue(val);
			upsertQuery.exec();
			qCDebug(logDbWatcher) << "Updated local data from cloud data for id" << key;
		} else {
			// delete -> delete it
			ExQuery deleteQuery{_db, ErrorScope::Entry, key};
			deleteQuery.prepare(QStringLiteral("DELETE FROM %1 "
											   "WHERE %2 == ?;")
									.arg(escTableName, fieldName(pKey)));
			deleteQuery.addBindValue(escKey);
			deleteQuery.exec();
			qCDebug(logDbWatcher) << "Removed local data for id" << key;
		}

		// update modified, mark unchanged -> now in sync with remote
		ExQuery markUnchangedQuery{_db, ErrorScope::Table, key.typeName};
		markUnchangedQuery.prepare(QStringLiteral("UPDATE %1 "
												  "SET changed = ?, tstamp = ? "
												  "WHERE pkey == ?;")
									   .arg(tableName(key.typeName, true)));
		markUnchangedQuery.addBindValue(static_cast<int>(ChangeState::Unchanged));
		markUnchangedQuery.addBindValue(data.modified().toUTC());
		markUnchangedQuery.addBindValue(escKey);
		markUnchangedQuery.exec();

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
	const auto &tInfo = _tables[name];
	try {
		ExTransaction transact{_db, name};

		const auto [pKey, pKeyType] = _tables[name].pKeyInfo;

		QStringList dataFields;
		if (tInfo.fields.isEmpty())
			dataFields.append(QStringLiteral("dataTable.*"));
		else {
			for (const auto &field : qAsConst(tInfo.fields))
				dataFields.append(QStringLiteral("dataTable.%1").arg(fieldName(field)));
		}
		ExQuery nextDataQuery{_db, ErrorScope::Table, name};
		nextDataQuery.prepare(QStringLiteral("SELECT syncTable.pkey, syncTable.tstamp, %1 "
											 "FROM %2 AS syncTable "
											 "LEFT JOIN %3 AS dataTable "
											 "ON syncTable.pkey == dataTable.%4 "
											 "WHERE syncTable.changed == ? "
											 "ORDER BY syncTable.tstamp ASC "
											 "LIMIT 1")
								  .arg(dataFields.join(QStringLiteral(", ")),
									   tableName(name, true),
									   tableName(name),
									   fieldName(pKey)));
		nextDataQuery.addBindValue(static_cast<int>(ChangeState::Changed));
		nextDataQuery.exec();

		if (nextDataQuery.first()) {
			LocalData data;
			data.setKey({name, encodeKey(nextDataQuery.value(0), pKeyType)});
			data.setModified(nextDataQuery.value(1).toDateTime().toUTC());
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
		ExTransaction transact{_db, key};

		const auto [pKey, pKeyType] = _tables[key.typeName].pKeyInfo;
		const auto escKey = decodeKey(key.id, pKeyType);

		ExQuery getStampQuery{_db, ErrorScope::Table, key.typeName};
		getStampQuery.prepare(QStringLiteral("SELECT tstamp "
											 "FROM %1 "
											 "WHERE pkey == ?")
								  .arg(tableName(key.typeName, true)));
		getStampQuery.addBindValue(escKey);
		getStampQuery.exec();
		if (getStampQuery.first()) {
			if (getStampQuery.value(0).toDateTime().toUTC() != modified.toUTC()) {
				qCDebug(logDbWatcher) << "Entry with id" << key
									  << "was modified after beeing uploaded (timestamp changed)";
				transact.commit();
				return;
			}
		}

		ExQuery markUnchangedQuery{_db, ErrorScope::Table, key.typeName};
		markUnchangedQuery.prepare(QStringLiteral("UPDATE %1 "
												  "SET changed = ? "
												  "WHERE pkey == ?;")
									   .arg(tableName(key.typeName, true)));
		markUnchangedQuery.addBindValue(static_cast<int>(ChangeState::Unchanged));
		markUnchangedQuery.addBindValue(escKey);
		markUnchangedQuery.exec();
		qCDebug(logDbWatcher) << "Updated metadata for id" << key;

		transact.commit();
	} catch (SqlException &error) {
		qCCritical(logDbWatcher) << error.what();
		error.emitFor(this);
	}
}

void DatabaseWatcher::markCorrupted(const ObjectKey &key, const QDateTime &modified)
{
	try {
		ExTransaction transact{_db, key};

		const auto [pKey, pKeyType] = _tables[key.typeName].pKeyInfo;
		const auto escKey = decodeKey(key.id, pKeyType);

		ExQuery insertCorruptedQuery{_db, ErrorScope::Table, key.typeName};
		insertCorruptedQuery.prepare(QStringLiteral("INSERT INTO %1 "
													"(pkey, tstamp, changed) "
													"VALUES(?, ?, ?) "
													"ON CONFLICT(pkey) DO UPDATE "
													"SET changed = excluded.changed "
													"WHERE pkey = excluded.pkey;")
										 .arg(tableName(key.typeName, true)));
		insertCorruptedQuery.addBindValue(escKey);
		insertCorruptedQuery.addBindValue(modified.toUTC());
		insertCorruptedQuery.addBindValue(static_cast<int>(ChangeState::Corrupted));
		insertCorruptedQuery.exec();

		transact.commit();
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

void DatabaseWatcher::createMetaTables(const QString &table)
{
	ExTransaction transact{_db, table};

	if (!_db.tables().contains(MetaTable)) {
		ExQuery createTableQuery{_db, ErrorScope::Database, table};
		createTableQuery.exec(QStringLiteral("CREATE TABLE %1 ( "
											 "	tableName	TEXT NOT NULL, "
											 "	pkeyName	TEXT NOT NULL, "
											 "	pkeyType	INTEGER NOT NULL, "
											 "	state		INTEGER NOT NULL DEFAULT 0 CHECK(state >= 0 AND state <= 2) , "
											 "	lastSync	TEXT, "
											 "	PRIMARY KEY(tableName) "
											 ") WITHOUT ROWID;")
								  .arg(MetaTable));
		qCInfo(logDbWatcher) << "Created sync metadata table";
	} else
		qCDebug(logDbWatcher) << "Sync metadata table already exist";

	if (!_db.tables().contains(FieldTable)) {
		ExQuery createTableQuery{_db, ErrorScope::Database, table};
		createTableQuery.exec(QStringLiteral("CREATE TABLE %1 ( "
											 "	tableName	TEXT NOT NULL, "
											 "	syncField	TEXT NOT NULL, "
											 "	PRIMARY KEY(tableName, syncField), "
											 "	FOREIGN KEY(tableName) REFERENCES %2(tableName) ON UPDATE CASCADE ON DELETE CASCADE"
											 ") WITHOUT ROWID;")
								  .arg(FieldTable, MetaTable));
		qCInfo(logDbWatcher) << "Created sync fields table";
	} else
		qCDebug(logDbWatcher) << "Sync fields table already exist";

	if (!_db.tables().contains(ReferenceTable)) {
		ExQuery createTableQuery{_db, ErrorScope::Database, table};
		createTableQuery.exec(QStringLiteral("CREATE TABLE %1 ( "
											 "	tableName	TEXT NOT NULL, "
											 "	fKeyTable	TEXT NOT NULL, "
											 "	fKeyField	TEXT NOT NULL, "
											 "	PRIMARY KEY(tableName, fKeyTable, fKeyField), "
											 "	FOREIGN KEY(tableName) REFERENCES %2(tableName) ON UPDATE CASCADE ON DELETE CASCADE"
											 ") WITHOUT ROWID;")
								  .arg(ReferenceTable, MetaTable));
		qCInfo(logDbWatcher) << "Created sync references table";
	} else
		qCDebug(logDbWatcher) << "Sync references table already exist";

	transact.commit();
}

DatabaseWatcher::PKeyInfo DatabaseWatcher::createSyncTables(const TableConfig &config)
{
	const auto name = config.table();
	const auto escName = tableName(name);
	const QString syncTableName = TablePrefix + name;
	const auto escSyncTableName = tableName(syncTableName);

	// obtain the primary key to use here
	auto pKey = config.primaryKey();
	if (pKey.isEmpty()) {
		const auto pIndex = _db.primaryIndex(name);
		if (pIndex.count() != 1) {
			qCCritical(logDbWatcher) << "Cannot synchronize table" << name
									 << "with composite primary key!";
			throw TableException{config.table(), QStringLiteral("Cannot synchronize table with composite primary key!"), _db.lastError()};
		}
		pKey = pIndex.fieldName(0);
	}
	const auto escPKey = fieldName(pKey);
	const auto pField = _db.record(name).field(pKey);

	if (!_db.tables().contains(syncTableName)) {
		const auto primaryType = sqlTypeName(pField);

		ExQuery createTableQuery{_db, ErrorScope::Table, name};
		createTableQuery.exec(QStringLiteral("CREATE TABLE %1 ( "
											 "	pkey	%2 NOT NULL, "
											 "	tstamp	TEXT NOT NULL, "
											 "	changed	INTEGER NOT NULL CHECK(changed >= 0 AND changed <= 2), "  // (unchanged, changed, corrupted)
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

	return std::make_pair(pKey, pField.type());
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

bool DatabaseWatcher::shouldStoreImpl(const LocalData &data, const QVariant &escKey)
{
	const auto key = data.key();
	ExQuery checkNewerQuery{_db, ErrorScope::Table, key.typeName};
	checkNewerQuery.prepare(QStringLiteral("SELECT tstamp "
										   "FROM %1 "
										   "WHERE pkey = ?;")
								.arg(tableName(key.typeName, true)));
	checkNewerQuery.addBindValue(escKey);
	checkNewerQuery.exec();
	if (checkNewerQuery.first()) {
		const auto localMod = checkNewerQuery.value(0).toDateTime().toUTC();
		if (localMod > data.modified()) {
			qCDebug(logDbWatcher) << "Data with id" << key
								  << "was modified in the cloud, but is newer locally."
								  << "Local date:" << localMod
								  << "Cloud date:" << data.modified();
			updateLastSync(key.typeName, data.uploaded());
			return false;
		} else
			return true;
	} else {
		qCDebug(logDbWatcher) << "No local data for id" << key;
		return true;
	}
}

QString DatabaseWatcher::encodeKey(const QVariant &key, int type) const
{
	// only handle SQLite types
	switch (type) {
	case QMetaType::Nullptr:
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
	case QMetaType::Double:
	case QMetaType::Float:
	case QMetaType::QString:
		return key.toString();
	case QMetaType::QByteArray:
	default:
		return QString::fromUtf8(key.toByteArray()
									 .toBase64(QByteArray::Base64Encoding |
											   QByteArray::KeepTrailingEquals));
	}
}

QVariant DatabaseWatcher::decodeKey(const QString &key, int type) const
{
	// only handle SQLite types
	switch (type) {
	case QMetaType::Nullptr:
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
	case QMetaType::Double:
	case QMetaType::Float:
	case QMetaType::QString: {
		QVariant var{key};
		if (var.convert(type))
			return var;
		else {
			qCWarning(logDbWatcher) << "Failed to convert primary key to type"
									<< QMetaType::typeName(type)
									<< "- falling back to using the string key!";
			return key;
		}
	}
	case QMetaType::QByteArray:
	default:
		return QByteArray::fromBase64(key.toUtf8(), QByteArray::Base64Encoding);
	}
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
	Q_EMIT watcher->databaseError(_scope, _key, _error);
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
