#include "databasewatcher_p.h"

#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlIndex>

using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logDbWatcher, "qt.datasync.DatabaseWatcher")

DatabaseWatcher::DatabaseWatcher(QSqlDatabase &&db, ITypeConverter *typeConverter, QObject *parent) :
	QObject{parent},
	_db{std::move(db)},
	_typeConverter{typeConverter}
{}

void DatabaseWatcher::addAllTables(QSql::TableType type = QSql::Tables)
{
	for (const auto &table : _db.tables(type)) {
		if (table.startsWith(QStringLiteral("__qtdatasync")))
			continue;
		addTable(table);
	}
}

void DatabaseWatcher::addTable(const QString &name, QString primaryType)
{
	try {
		// step 1: create the sync attachment table, if not existant yet
		const QString syncTableName = QStringLiteral("__qtdatasync_sync_data_") + name;
		if (!_db.tables().contains(syncTableName)) {
			ExTransaction transAct{_db};

			const auto pKey = _db.primaryIndex(name).fieldName(0);
			if (primaryType.isEmpty())
				primaryType = _typeConverter->sqlTypeName(_db.record(name).field(pKey));

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

			transAct.commit();
			qCInfo(logDbWatcher) << "Created synchronization tables for table" << name;
		} else
			qCDebug(logDbWatcher) << "Sync tables for table" << name << "already exist";
	} catch (QSqlError &error) {
		qCCritical(logDbWatcher).noquote() << error.text();
		Q_EMIT databaseError(_db.connectionName(), error.text(), {});
	}
}



DatabaseWatcher::ITypeConverter::ITypeConverter() = default;

DatabaseWatcher::ITypeConverter::~ITypeConverter() = default;



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



QString SqliteTypeConverter::sqlTypeName(const QSqlField &field) const
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
