#include "databasewatcher_p.h"

#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlIndex>

using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logDbWatcher, "qt.datasync.DatabaseWatcher")

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
		// step 1: create the sync attachment table, if not existant yet
		const QString syncTableName = TablePrefix + name;
		if (!_db.tables().contains(syncTableName)) {
			ExTransaction transact{_db};

			const auto pKey = _db.primaryIndex(name).fieldName(0);
			if (primaryType.isEmpty())
				primaryType = sqlTypeName(_db.record(name).field(pKey));

			ExQuery createTableQuery{_db};
			createTableQuery.exec(QStringLiteral("CREATE TABLE \"%1\" ( "
												 "	pkey	%2 NOT NULL, "
												 "	version	INTEGER NOT NULL, "
												 "	changed	INTEGER NOT NULL CHECK(changed >= 0 AND changed <= 2), "  // (unchanged, changed, deleted)
												 "PRIMARY KEY(pkey));")
									  .arg(syncTableName, primaryType));
			qCDebug(logDbWatcher) << "Created sync table for" << name;

			ExQuery createIndexQuery{_db};
			createIndexQuery.exec(QStringLiteral("CREATE INDEX \"%1_idx_changed\" ON \"%1\" (changed);")
									  .arg(syncTableName));
			qCDebug(logDbWatcher) << "Created sync table changed index for" << name;

			ExQuery createInsertTrigger{_db};
			createInsertTrigger.exec(QStringLiteral("CREATE TRIGGER \"%1_insert_trigger\" "
													"AFTER INSERT ON \"%2\" "
													"FOR EACH ROW "
													"BEGIN "
														// add a default value - but only if not exists yet
													"	INSERT OR IGNORE INTO \"%1\" "
													"	(pkey, version, changed) "
													"	VALUES(NEW.\"%3\", 0, 0); "  // 0 == unchanged
														// then set version and changed, works for both, existing and created
													"	UPDATE \"%1\" "
													"	SET version = version + 1, changed = 1 "  // 1 == changed
													"	WHERE pkey = NEW.\"%3\"; "  // 1 == changed
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
													"	SET version = version + 1, changed = 1 "  // 1 == changed
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
													"	SET version = version + 1, changed = 2 "  // 2 == deleted
													"	WHERE pkey = OLD.\"%3\"; "
														// then insert (or replace) new entry with new id but same version as deleted
													"	INSERT OR REPLACE INTO \"%1\" "
													"	(pkey, version, changed) "
													"	VALUES(NEW.\"%3\", ( "
													"		SELECT version FROM \"%1\" WHERE pKey = OLD.\"%3\" "
													"	), 1); "  // 1 == changed
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
													"	SET version = version + 1, changed = 2 "  // 2 == deleted
													"	WHERE pkey = OLD.\"%3\"; "
													"END;")
										 .arg(syncTableName, name, pKey));
			qCDebug(logDbWatcher) << "Created sync table delete trigger index for" << name;

			ExQuery inflateTableTrigger{_db};
			inflateTableTrigger.exec(QStringLiteral("INSERT INTO \"%1\" "
													"(pkey, version, changed) "
													"SELECT \"%3\", 1, 1 FROM \"%2\";")
										 .arg(syncTableName, name, pKey));
			qCDebug(logDbWatcher) << "Inflated sync table for" << name
								  << "with current data of that table";

			transact.commit();
			qCInfo(logDbWatcher) << "Created synchronization tables for table" << name;
		} else
			qCDebug(logDbWatcher) << "Sync tables for table" << name << "already exist";

		// step 2: register notification hook for the sync table
		_tables.insert(name, fields);
		if (!_db.driver()->subscribeToNotification(syncTableName))
			qCWarning(logDbWatcher) << "Unable to register update notification hook for sync table for" << name;  // no hard error, as data is still synced, just not live

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
	if (removeRef)
		_tables.remove(name);
}

void DatabaseWatcher::unsyncAllTables()
{
	for (auto it = _tables.keyBegin(); it != _tables.keyEnd(); ++it)
		unsyncTable(*it, false);
	_tables.clear();
}

void DatabaseWatcher::unsyncTable(const QString &name, bool removeRef)
{
	try {
		// step 1: remove table
		removeTable(name, removeRef);

		// step 2: remove all sync stuff
		const QString syncTableName = TablePrefix + name;
		if (_db.tables().contains(syncTableName)) {
			ExTransaction transact{_db};

			ExQuery dropDeleteTrigger{_db};
			dropDeleteTrigger.exec(QStringLiteral("DROP TRIGGER \"%1_delete_trigger\"").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table delete trigger index for" << name;

			ExQuery dropRenameTrigger{_db};
			dropRenameTrigger.exec(QStringLiteral("DROP TRIGGER \"%1_rename_trigger\"").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table rename trigger index for" << name;

			ExQuery dropUpdateTrigger{_db};
			dropUpdateTrigger.exec(QStringLiteral("DROP TRIGGER \"%1_update_trigger\"").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table update trigger index for" << name;

			ExQuery dropInsertTrigger{_db};
			dropInsertTrigger.exec(QStringLiteral("DROP TRIGGER \"%1_insert_trigger\"").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table insert trigger index for" << name;

			ExQuery dropIndexQuery{_db};
			dropIndexQuery.exec(QStringLiteral("DROP INDEX \"%1_idx_changed\"").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table index for" << name;

			ExQuery dropTableQuery{_db};
			dropTableQuery.exec(QStringLiteral("DROP TABLE \"%1\"").arg(syncTableName));
			qCDebug(logDbWatcher) << "Dropped sync table for" << name;

			transact.commit();
			qCInfo(logDbWatcher) << "Dropped synchronization tables for table" << name;
		} else
			qCDebug(logDbWatcher) << "Sync tables for table" << name << "do not exist";
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
