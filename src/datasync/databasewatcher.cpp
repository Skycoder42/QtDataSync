#include "databasewatcher_p.h"
#include "engine.h"

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

void DatabaseWatcher::reactivateTables()
{
	Q_UNIMPLEMENTED();
}

void DatabaseWatcher::addAllTables(QSql::TableType type)
{
	for (const auto &table : _db.tables(type)) {
		if (table.startsWith(TablePrefix))
			continue;
		addTable(table);
	}
}

void DatabaseWatcher::addTable(const QString &name, const QStringList &fields, QString primaryType)
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
			ExQuery createTableQuery{_db};
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
		ExTransaction transact{_db};
		// step 2.1: create the sync attachment table, if not existant yet
		const auto escName = tableName(name);
		const auto escSyncTableName = tableName(syncTableName);
		if (!_db.tables().contains(syncTableName)) {
			if (primaryType.isEmpty())
				primaryType = sqlTypeName(_db.record(name).field(pKey));

			ExQuery createTableQuery{_db};
			createTableQuery.exec(QStringLiteral("CREATE TABLE %1 ( "
												 "	pkey	%2 NOT NULL, "
												 "	tstamp	TEXT NOT NULL, "
												 "	changed	INTEGER NOT NULL CHECK(changed >= 0 AND changed <= 2), "  // (unchanged, changed, deleted)
												 "	PRIMARY KEY(pkey)"
												 ");")
									  .arg(escSyncTableName, primaryType));
			qCDebug(logDbWatcher) << "Created sync table for" << name;

			ExQuery createIndexQuery{_db};
			createIndexQuery.exec(QStringLiteral("CREATE INDEX %1 ON %2 (changed ASC);")
									  .arg(tableName(syncTableName + QStringLiteral("_idx_changed")),
										   escSyncTableName));
			qCDebug(logDbWatcher) << "Created sync table changed index for" << name;

			ExQuery createInsertTrigger{_db};
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

			ExQuery createUpdateTrigger{_db};
			createUpdateTrigger.exec(QStringLiteral("CREATE TRIGGER %1 "
													"AFTER UPDATE ON %3 "
													"FOR EACH ROW "
													"WHEN NEW.%4 == OLD.%4 "
													"BEGIN "
														// set version and changed
													"	UPDATE %2 "
													"	SET tstamp = strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), changed = 1 "  // 1 == changed
													"	WHERE pkey = NEW.%3; "
													"END;")
										 .arg(tableName(syncTableName + QStringLiteral("_update_trigger")),
											  escSyncTableName,
											  escName,
											  escPKey));
			qCDebug(logDbWatcher) << "Created sync table update trigger index for" << name;

			ExQuery createRenameTrigger{_db};
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

			ExQuery createDeleteTrigger{_db};
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

			ExQuery inflateTableTrigger{_db};
			inflateTableTrigger.exec(QStringLiteral("INSERT INTO %1 "
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
		ExQuery addMetaDataQuery{_db};
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
		qCInfo(logDbWatcher) << "Created metadata and enabled synchronization for table" << name;

		// step 2.3: commit transaction
		transact.commit();

		// step 3: register notification hook for the sync table
		_tables.insert(name, fields);
		if (!_db.driver()->subscribeToNotification(syncTableName))
			qCWarning(logDbWatcher) << "Unable to register update notification hook for sync table for" << name;  // no hard error, as data is still synced, just not live

		// step 4: emit signals
		Q_EMIT tableAdded(name);
		Q_EMIT triggerSync(name);
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to synchronize table" << name
								 << "with error:" << qUtf8Printable(error.text());
		throw TableException{name, QStringLiteral("Failed to synchronize table with database error"), error};
	}
}

void DatabaseWatcher::removeAllTables()
{
	for (auto it = _tables.keyBegin(); it != _tables.keyEnd(); ++it)
		removeTable(*it, false);
	_tables.clear();
}

void DatabaseWatcher::removeTable(const QString &name, bool removeRef)
{
	_db.driver()->unsubscribeFromNotification(TablePrefix + name);

	try {
		ExQuery removeMetaData{_db};
		removeMetaData.prepare(QStringLiteral("UPDATE %1 "
											  "SET state = ? "
											  "WHERE tableName == ?;")
								   .arg(MetaTable));
		removeMetaData.addBindValue(static_cast<int>(TableState::Inactive));
		removeMetaData.addBindValue(name);
		removeMetaData.exec();
		qCInfo(logDbWatcher) << "Disabled synchronization for" << name;
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to remove metadata for table" << name
								 << "with error:" << qUtf8Printable(error.text());
		throw TableException{name, QStringLiteral("Failed to deactivate synchronization for table with database error"), error};
	}

	if (removeRef)
		_tables.remove(name);

	Q_EMIT tableRemoved(name);
}

void DatabaseWatcher::unsyncAllTables()
{
	for (auto it = _tables.keyBegin(); it != _tables.keyEnd(); ++it)
		unsyncTable(*it, false);
	_tables.clear();

	try {
		ExTransaction transact{_db};

		// step 1: check if metadata table is empty
		ExQuery checkMetaQuery{_db};
		checkMetaQuery.exec(QStringLiteral("SELECT COUNT(*) FROM %1;")
								.arg(MetaTable));

		// step 2: if empty -> drop the table
		if (checkMetaQuery.first() && checkMetaQuery.value(0) == 0) {
			ExQuery dropMetaTable{_db};
			dropMetaTable.exec(QStringLiteral("DROP TABLE %1;").arg(MetaTable));
			qCInfo(logDbWatcher) << "Dropped all synchronization data";
		}

		// step 3: commit
		transact.commit();
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to drop empty metadata table with error:"
								 << qUtf8Printable(error.text());
		throw TableException{{}, QStringLiteral("Failed clear metadata with database error"), error};
	}
}

void DatabaseWatcher::unsyncTable(const QString &name, bool removeRef)
{
	try {
		// step 1: remove table
		removeTable(name, removeRef);

		// step 2: remove all sync stuff
		ExTransaction transact{_db};

		// step 2.1 remove sync metadata
		ExQuery removeMetaData{_db};
		removeMetaData.prepare(QStringLiteral("DELETE FROM %1 "
											  "WHERE tableName == ?;")
								   .arg(MetaTable));
		removeMetaData.addBindValue(name);
		removeMetaData.exec();
		qCDebug(logDbWatcher) << "Removed metadata for" << name;

		// step 2.2 remove specific sync tables
		const QString syncTableName = TablePrefix + name;
		if (_db.tables().contains(syncTableName)) {
			ExQuery dropDeleteTrigger{_db};
			dropDeleteTrigger.exec(QStringLiteral("DROP TRIGGER %1;").arg(tableName(syncTableName + QStringLiteral("_delete_trigger"))));
			qCDebug(logDbWatcher) << "Dropped sync table delete trigger index for" << name;

			ExQuery dropRenameTrigger{_db};
			dropRenameTrigger.exec(QStringLiteral("DROP TRIGGER %1;").arg(tableName(syncTableName + QStringLiteral("_rename_trigger"))));
			qCDebug(logDbWatcher) << "Dropped sync table rename trigger index for" << name;

			ExQuery dropUpdateTrigger{_db};
			dropUpdateTrigger.exec(QStringLiteral("DROP TRIGGER %1;").arg(tableName(syncTableName + QStringLiteral("_update_trigger"))));
			qCDebug(logDbWatcher) << "Dropped sync table update trigger index for" << name;

			ExQuery dropInsertTrigger{_db};
			dropInsertTrigger.exec(QStringLiteral("DROP TRIGGER %1;").arg(tableName(syncTableName + QStringLiteral("_insert_trigger"))));
			qCDebug(logDbWatcher) << "Dropped sync table insert trigger index for" << name;

			ExQuery dropIndexQuery{_db};
			dropIndexQuery.exec(QStringLiteral("DROP INDEX %1;").arg(tableName(syncTableName + QStringLiteral("_idx_changed"))));
			qCDebug(logDbWatcher) << "Dropped sync table index for" << name;

			ExQuery dropTableQuery{_db};
			dropTableQuery.exec(QStringLiteral("DROP TABLE %1;").arg(tableName(syncTableName)));
			qCDebug(logDbWatcher) << "Dropped sync table for" << name;
		} else
			qCDebug(logDbWatcher) << "Sync tables for table" << name << "do not exist";

		// step 2.3 commit transaction
		transact.commit();
		qCInfo(logDbWatcher) << "Dropped synchronization data for table" << name;
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to drop synchronization tables for table" << name
								 << "with error:" << qUtf8Printable(error.text());
		throw TableException{name, QStringLiteral("Failed to unsynchronize table with database error"), error};
	}
}

std::optional<QDateTime> DatabaseWatcher::lastSync(const QString &tableName)
{
	try {
		ExQuery lastSyncQuery{_db};
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
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to fetch change data for table" << tableName
								 << "with error:" << qUtf8Printable(error.text());
		Q_EMIT databaseError(ErrorScope::Database,
							 tr("Failed to get last synced time for table %1").arg(tableName),
							 MetaTable,
							 error);
		return std::nullopt;
	}
}

void DatabaseWatcher::storeData(const LocalData &data)
{
	const auto key = data.key();
	try {
		ExTransaction transact{_db};

		// retrieve the primary key
		const auto pKey = getPKey(key.typeName);
		if (!pKey)
			return;
		const auto escTableName = tableName(key.typeName);
		const auto escSyncName = tableName(key.typeName, true);

		// check if newer
		try {
			ExQuery checkNewerQuery{_db};
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
					transact.commit();
					return;
				}
			} else
				qCDebug(logDbWatcher) << "No local data for id" << key;
		} catch (QSqlError &error) {
			transact.rollback();
			Q_EMIT databaseError(ErrorScope::Table,
								 tr("Failed to access metadata table for %1").arg(key.typeName),
								 QString{TablePrefix + key.typeName},
								 error);
			return;
		}

		// update the actual data
		try {
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
				ExQuery upsertQuery{_db};
				upsertQuery.prepare(QStringLiteral("INSERT INTO %1 "
												   "(%2) "
												   "VALUES(%3) "
												   "ON CONFLICT(%4) DO UPDATE "
												   "SET %5 "
												   "WHERE %4 = excluded.%4;")
										.arg(escTableName,
											 insertKeys.join(QStringLiteral(", ")),
											 paramPlcs.join(QStringLiteral(", ")),
											 *pKey,
											 updateKeys.join(QStringLiteral(", "))));
				for (const auto &val : *value)
					upsertQuery.addBindValue(val);
				upsertQuery.exec();
				qCDebug(logDbWatcher) << "Updated local data from cloud data for id" << key;
			} else {
				// delete -> delete it
				ExQuery deleteQuery{_db};
				deleteQuery.prepare(QStringLiteral("DELETE FROM %1 "
												   "WHERE %2 == ?;")
										.arg(escTableName, *pKey));
				deleteQuery.addBindValue(key.id);
				deleteQuery.exec();
				qCDebug(logDbWatcher) << "Removed local data for id" << key;
			}
		} catch (QSqlError &error) {
			transact.rollback();
			markCorrupted(key, data.modified());
			Q_EMIT databaseError(ErrorScope::Entry,
								 tr("Failed to update local data for %1").arg(key.toString()),
								 QVariant::fromValue(key),
								 error);
			return;
		}
		// update modified, mark unchanged -> now in sync with remote
		markUnchanged(key, data.modified());

		// update last sync
		ExQuery updateMetaQuery{_db};
		updateMetaQuery.prepare(QStringLiteral("UPDATE %1 "
											   "SET lastSync = ? "
											   "WHERE tableName = ?;")
									.arg(MetaTable));
		updateMetaQuery.addBindValue(data.uploaded().toUTC());
		updateMetaQuery.addBindValue(key.typeName);
		updateMetaQuery.exec();

		transact.commit();
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to store local data for id" << key
								 << "with error:" << qUtf8Printable(error.text());
		markCorrupted(key, data.modified());
		Q_EMIT databaseError(tr("Failed to store data with id %1 locally in table").arg(key.toString()));
	}
}

std::optional<LocalData> DatabaseWatcher::loadData(const QString &name)
{
	try {
		ExTransaction transact{_db};

		const auto pKey = getPKey(name);
		if (!pKey)
			return std::nullopt;

		ExQuery nextDataQuery{_db};
		nextDataQuery.prepare(QStringLiteral("SELECT syncTable.pkey, syncTable.tstamp, dataTable.* "
											 "FROM %1 AS syncTable "
											 "LEFT JOIN %2 AS dataTable "
											 "ON syncTable.pkey == dataTable.%3 "
											 "WHERE syncTable.changed != ? "
											 "ORDER BY syncTable.tstamp ASC "
											 "LIMIT 1")
								  .arg(tableName(name, true),
									   tableName(name),
									   *pKey));
		nextDataQuery.addBindValue(static_cast<int>(ChangeState::Unchanged));
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
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to load next local data for type" << name
								 << "with error:" << qUtf8Printable(error.text());
		// TODO markCorrupted(name);
		Q_EMIT databaseError(tr("Failed to load local data from table %1").arg(name));
		return std::nullopt;
	}
}

void DatabaseWatcher::markUnchanged(const ObjectKey &key, const QDateTime &modified)
{
	try {
		ExQuery markUnchangedQuery{_db};
		markUnchangedQuery.prepare(QStringLiteral("UPDATE %1 "
												  "SET changed = ?, tstamp = ? "
												  "WHERE pkey = ?;")
									   .arg(tableName(key.typeName, true)));
		markUnchangedQuery.addBindValue(static_cast<int>(ChangeState::Unchanged));
		markUnchangedQuery.addBindValue(modified.toUTC());
		markUnchangedQuery.addBindValue(key.id);
		markUnchangedQuery.exec();
		qCDebug(logDbWatcher) << "Updated metadata for id" << key;
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to mark entry with id" << key
								 << "as unchanged with error" << qUtf8Printable(error.text());
		Q_EMIT databaseError(ErrorScope::Table,
							 tr("Failed to access metadata table for %1").arg(key.typeName),
							 QString{TablePrefix + key.typeName},
							 error);
	}
}

void DatabaseWatcher::markCorrupted(const ObjectKey &key, const QDateTime &modified)
{
	try {
		ExQuery insertCorruptedQuery{_db};
		insertCorruptedQuery.prepare(QStringLiteral("INSERT INTO %1 "
													"(pkey, tstamp, changed) "
													"VALUES(?, ?, ?) "
													"ON CONFLICT(pkey) DO UPDATE "
													"SET changed = excluded.changed "
													"WHERE pkey = excluded.pkey;"));
		insertCorruptedQuery.addBindValue(key.id);
		insertCorruptedQuery.addBindValue(modified.toUTC());
		insertCorruptedQuery.addBindValue(static_cast<int>(ChangeState::Corrupted));
		insertCorruptedQuery.exec();
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to mark entry with id" << key
								 << "as corrupted with error" << qUtf8Printable(error.text());
	}
}

void DatabaseWatcher::dbNotify(const QString &name)
{
	if (name.size() <= TablePrefix.size())
		return;
	const auto origName = name.mid(TablePrefix.size());
	if (_tables.contains(origName))
		Q_EMIT triggerSync(origName);
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

std::optional<QString> DatabaseWatcher::getPKey(const QString &table)
{
	try {
		ExQuery getPKeyQuery{_db};
		getPKeyQuery.prepare(QStringLiteral("SELECT pkey "
											"FROM %1 "
											"WHERE tableName = ?;")
								 .arg(MetaTable));
		getPKeyQuery.addBindValue(table);  // TODO add cache to reduce queries
		getPKeyQuery.exec();
		if (!getPKeyQuery.first()) {
			Q_EMIT databaseError(ErrorScope::Database,
								 tr("No primary registered for table %1").arg(table),
								 MetaTable,
								 {});
			return std::nullopt;
		}

		return fieldName(getPKeyQuery.value(0).toString());
	} catch (QSqlError &error) {
		Q_EMIT databaseError(ErrorScope::Database,
							 tr("Failed to retrieve primary key for table %1").arg(table),
							 MetaTable,
							 error);
		return std::nullopt;
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

QString SqlException::message() const
{
	switch (_scope) {
	case ErrorScope::Entry:
		return DatabaseWatcher::tr("Failed to modify local data %1").arg(_key.value<ObjectKey>().toString());
	case ErrorScope::Table:
	case ErrorScope::Database:
	case ErrorScope::System:
	}
}

void SqlException::raise() const
{

}

ExceptionBase *SqlException::clone() const
{

}

QString SqlException::qWhat() const
{

}



ExQuery::ExQuery(QSqlDatabase db) :
	QSqlQuery{db}
{}

void ExQuery::prepare(const QString &query)
{
	if (!QSqlQuery::prepare(query))
		throw lastError();
}

void ExQuery::exec()
{
	if (!QSqlQuery::exec())
		throw lastError();
}

void ExQuery::exec(const QString &query)
{
	if (!QSqlQuery::exec(query))
		throw lastError();
}

ExTransaction::ExTransaction() = default;

ExTransaction::ExTransaction(QSqlDatabase db) :
	_db{db}
{
	if (!_db->transaction())
		throw _db->lastError();
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
		throw _db->lastError();
	_db.reset();
}

void ExTransaction::rollback()
{
	_db->rollback();
	_db.reset();
}
