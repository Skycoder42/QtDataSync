#include "databasewatcher_p.h"

#include <QtSql/QSqlError>
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

bool DatabaseWatcher::addAllTables(QSql::TableType type)
{
	for (const auto &table : _db.tables(type)) {
		if (table.startsWith(TablePrefix))
			continue;
		if (!addTable(table))
			return false;
	}
	return true;
}

bool DatabaseWatcher::addTable(const QString &name, const QStringList &fields, QString primaryType)
{
	try {
		// step 0: verify primary key
		const auto pIndex = _db.primaryIndex(name);
		if (pIndex.count() != 1) {
			qCCritical(logDbWatcher) << "Cannot synchronize table" << name
									 << "with composite primary key!";
			return false;
		}
		const auto pKey = pIndex.fieldName(0);

		// step 1: create the global sync table
		if (!_db.tables().contains(MetaTable)) {
			ExQuery createTableQuery{_db};
			createTableQuery.exec(QStringLiteral("CREATE TABLE \"%1\" ( "
												 "	tableName	TEXT NOT NULL, "
												 "	pkey		TEXT NOT NULL, "
												 "	active		INTEGER NOT NULL DEFAULT 0 CHECK(active == 0 OR active == 1) , "
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
		const QString syncTableName = TablePrefix + name;
		if (!_db.tables().contains(syncTableName)) {

			if (primaryType.isEmpty())
				primaryType = sqlTypeName(_db.record(name).field(pKey));

			ExQuery createTableQuery{_db};
			createTableQuery.exec(QStringLiteral("CREATE TABLE \"%1\" ( "
												 "	pkey	%2 NOT NULL, "
												 "	tstamp	TEXT NOT NULL, "
												 "	changed	INTEGER NOT NULL CHECK(changed >= 0 AND changed <= 2), "  // (unchanged, changed, deleted)
												 "	PRIMARY KEY(pkey)"
												 ");")
									  .arg(syncTableName, primaryType));
			qCDebug(logDbWatcher) << "Created sync table for" << name;

			ExQuery createIndexQuery{_db};
			createIndexQuery.exec(QStringLiteral("CREATE INDEX \"%1_idx_changed\" ON \"%1\" (changed ASC);")
									  .arg(syncTableName));
			qCDebug(logDbWatcher) << "Created sync table changed index for" << name;

			ExQuery createInsertTrigger{_db};
			createInsertTrigger.exec(QStringLiteral("CREATE TRIGGER \"%1_insert_trigger\" "
													"AFTER INSERT ON \"%2\" "
													"FOR EACH ROW "
													"BEGIN "
														// add or replace, in case deleted still exists
													"	INSERT OR REPLACE INTO \"%1\" "
													"	(pkey, tstamp, changed) "
													"	VALUES(NEW.\"%3\", strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), 1); "  // 1 == changed
													"END;")
									 .arg(syncTableName, name, pKey));
			qCDebug(logDbWatcher) << "Created sync table insert trigger index for" << name;

			ExQuery createUpdateTrigger{_db};
			createUpdateTrigger.exec(QStringLiteral("CREATE TRIGGER \"%1_update_trigger\" "
													"AFTER UPDATE ON \"%2\" "
													"FOR EACH ROW "
													"WHEN NEW.\"%3\" == OLD.\"%3\" "
													"BEGIN "
														// set version and changed
													"	UPDATE \"%1\" "
													"	SET tstamp = strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), changed = 1 "  // 1 == changed
													"	WHERE pkey = NEW.\"%3\"; "
													"END;")
										 .arg(syncTableName, name, pKey));
			qCDebug(logDbWatcher) << "Created sync table update trigger index for" << name;

			ExQuery createRenameTrigger{_db};
			createRenameTrigger.exec(QStringLiteral("CREATE TRIGGER \"%1_rename_trigger\" "
													"AFTER UPDATE ON \"%2\" "
													"FOR EACH ROW "
													"WHEN NEW.\"%3\" != OLD.\"%3\" "
													"BEGIN "
														// mark old id as deleted
													"	UPDATE \"%1\" "
													"	SET tstamp = strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), changed = 2 "  // 2 == deleted
													"	WHERE pkey = OLD.\"%3\"; "
														// then insert (or replace) new entry with new id but same timestamp
													"	INSERT OR REPLACE INTO \"%1\" "
													"	(pkey, tstamp, changed) "
													"	VALUES(NEW.\"%3\", strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), 1); "  // 1 == changed
													"END;")
										 .arg(syncTableName, name, pKey));
			qCDebug(logDbWatcher) << "Created sync table rename trigger index for" << name;

			ExQuery createDeleteTrigger{_db};
			createDeleteTrigger.exec(QStringLiteral("CREATE TRIGGER \"%1_delete_trigger\" "
													"AFTER DELETE ON \"%2\" "
													"FOR EACH ROW "
													"BEGIN "
														// set version and changed
													"	UPDATE \"%1\" "
													"	SET tstamp = strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), changed = 2 "  // 2 == deleted
													"	WHERE pkey = OLD.\"%3\"; "
													"END;")
										 .arg(syncTableName, name, pKey));
			qCDebug(logDbWatcher) << "Created sync table delete trigger index for" << name;

			ExQuery inflateTableTrigger{_db};
			inflateTableTrigger.exec(QStringLiteral("INSERT INTO \"%1\" "
													"(pkey, tstamp, changed) "
													"SELECT \"%3\", strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), 1 "
													"FROM \"%2\";")
										 .arg(syncTableName, name, pKey));
			qCDebug(logDbWatcher) << "Inflated sync table for" << name
								  << "with current data of that table";

			qCInfo(logDbWatcher) << "Created synchronization tables for table" << name;
		} else
			qCDebug(logDbWatcher) << "Sync tables for table" << name << "already exist";

		// step 2.2: add to metadata table
		ExQuery addMetaDataQuery{_db};
		addMetaDataQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO \"%1\" "
												"(tableName, pkey, active, lastSync) "
												"VALUES(?, ?, ?, NULL);")
								 .arg(MetaTable));
		addMetaDataQuery.addBindValue(name);
		addMetaDataQuery.addBindValue(pKey);
		addMetaDataQuery.addBindValue(true);
		addMetaDataQuery.exec();
		if (addMetaDataQuery.numRowsAffected() == 0) { // ignored -> update instead
			ExQuery updateMetaDataQuery{_db};
			updateMetaDataQuery.prepare(QStringLiteral("UPDATE \"%1\" "
													   "SET active = ? "
													   "WHERE tableName == ?;")
											.arg(MetaTable));
			updateMetaDataQuery.addBindValue(true);
			updateMetaDataQuery.addBindValue(name);
			updateMetaDataQuery.exec();
			qCInfo(logDbWatcher) << "Enabled synchronization for table" << name;
		} else
			qCInfo(logDbWatcher) << "Created metadata and enabled synchronization for table" << name;

		// step 2.3: commit transaction
		transact.commit();

		// step 3: register notification hook for the sync table
		_tables.insert(name, fields);
		if (!_db.driver()->subscribeToNotification(syncTableName))
			qCWarning(logDbWatcher) << "Unable to register update notification hook for sync table for" << name;  // no hard error, as data is still synced, just not live

		// step 4: emit sync triggered
		Q_EMIT triggerSync(name, {});

		return true;
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to synchronize table" << name
								 << "with error:" << qUtf8Printable(error.text());
		return false;
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
		removeMetaData.prepare(QStringLiteral("UPDATE \"%1\" "
											  "SET active = ? "
											  "WHERE tableName == ?;"));
		removeMetaData.addBindValue(false);
		removeMetaData.addBindValue(name);
		removeMetaData.exec();
		qCInfo(logDbWatcher) << "Disabled synchronization for" << name;
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to remove metadata for table" << name
								 << "with error:" << qUtf8Printable(error.text());
	}

	if (removeRef)
		_tables.remove(name);
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
		checkMetaQuery.exec(QStringLiteral("SELECT COUNT(*) FROM \"%1\";")
								.arg(MetaTable));

		// step 2: if empty -> drop the table
		if (checkMetaQuery.first() && checkMetaQuery.value(0) == 0) {
			ExQuery dropMetaTable{_db};
			dropMetaTable.exec(QStringLiteral("DROP TABLE \"%1\";").arg(MetaTable));
			qCInfo(logDbWatcher) << "Dropped all synchronization data";
		}

		// step 3: commit
		transact.commit();
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to drop empty metadata table with error:"
								 << qUtf8Printable(error.text());
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
		removeMetaData.prepare(QStringLiteral("DELETE FROM \"%1\" "
											  "WHERE tableName == ?;"));
		removeMetaData.addBindValue(name);
		removeMetaData.exec();
		qCDebug(logDbWatcher) << "Removed metadata for" << name;

		// step 2.2 remove specific sync tables
		const QString syncTableName = TablePrefix + name;
		if (_db.tables().contains(syncTableName)) {
			ExQuery dropDeleteTrigger{_db};
			dropDeleteTrigger.exec(QStringLiteral("DROP TRIGGER \"%1_delete_trigger\";").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table delete trigger index for" << name;

			ExQuery dropRenameTrigger{_db};
			dropRenameTrigger.exec(QStringLiteral("DROP TRIGGER \"%1_rename_trigger\";").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table rename trigger index for" << name;

			ExQuery dropUpdateTrigger{_db};
			dropUpdateTrigger.exec(QStringLiteral("DROP TRIGGER \"%1_update_trigger\";").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table update trigger index for" << name;

			ExQuery dropInsertTrigger{_db};
			dropInsertTrigger.exec(QStringLiteral("DROP TRIGGER \"%1_insert_trigger\";").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table insert trigger index for" << name;

			ExQuery dropIndexQuery{_db};
			dropIndexQuery.exec(QStringLiteral("DROP INDEX \"%1_idx_changed\";").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table index for" << name;

			ExQuery dropTableQuery{_db};
			dropTableQuery.exec(QStringLiteral("DROP TABLE \"%1\";").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table for" << name;
		} else
			qCDebug(logDbWatcher) << "Sync tables for table" << name << "do not exist";

		// step 2.3 commit transaction
		transact.commit();
		qCInfo(logDbWatcher) << "Dropped synchronization data for table" << name;
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to drop synchronization tables for table" << name
								 << "with error:" << qUtf8Printable(error.text());
	}
}

quint64 DatabaseWatcher::changeCount() const
{
	return 0;
}

bool DatabaseWatcher::processChanges(const QString &tableName, const std::function<void(QVariant, quint64, ChangeState)> &callback, int limit)
{
	try {
		const QString syncTableName = TablePrefix + tableName;
		ExQuery changeQuery{_db};
		changeQuery.prepare(QStringLiteral("SELECT pkey, version, changed "
										   "FROM \"%1\" "
										   "WHERE changed != 0 "
										   "LIMIT ?")
								.arg(syncTableName));
		changeQuery.addBindValue(limit);
		changeQuery.exec();

		while (changeQuery.next()) {
			callback(changeQuery.value(0),
					 changeQuery.value(1).toULongLong(),
					 static_cast<ChangeState>(changeQuery.value(2).toInt()));
		}
		return changeQuery.size() > 0;
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher) << "Failed to fetch change data for table" << tableName
								 << "with error:" << qUtf8Printable(error.text());
		return false;
	}
}

void DatabaseWatcher::dbNotify(const QString &name)
{
	if (name.size() <= TablePrefix.size())
		return;
	const auto origName = name.mid(TablePrefix.size());
	if (_tables.contains(origName))
		Q_EMIT triggerSync(origName, {});
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
