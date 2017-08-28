#include "defaults.h"
#include "sqlstateholder_p.h"

#include <QtCore/QDebug>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

using namespace QtDataSync;

#define QTDATASYNC_LOG logger

SqlStateHolder::SqlStateHolder(QObject *parent) :
	StateHolder(parent),
	defaults(nullptr),
	logger(nullptr),
	database()
{}

void SqlStateHolder::initialize(Defaults *defaults)
{
	this->defaults = defaults;
	logger = defaults->createLogger("stateholder", this);
	database = defaults->aquireDatabase();

	//create table
	if(!database.tables().contains(QStringLiteral("SyncState"))) {
		QSqlQuery createQuery(database);
		createQuery.prepare(QStringLiteral("CREATE TABLE SyncState ("
												"Type		TEXT NOT NULL,"
												"Key		TEXT NOT NULL,"
												"Changed	INTEGER NOT NULL CHECK(Changed >= 0 AND Changed <= 2),"
												"PRIMARY KEY(Type, Key)"
										   ");"));
		if(!createQuery.exec()) {
			logFatal(false,
					 QStringLiteral("Failed to create SyncState table with error:\n") +
					 createQuery.lastError().text());
		}
	}
}

void SqlStateHolder::finalize()
{
	database = QSqlDatabase();
	defaults->releaseDatabase();
}

StateHolder::ChangeHash SqlStateHolder::listLocalChanges()
{
	QSqlQuery listQuery(database);
	listQuery.prepare(QStringLiteral("SELECT Type, Key, Changed FROM SyncState WHERE Changed != 0"));
	if(!listQuery.exec()) {
		logFatal(true,
				 QStringLiteral("Failed to load current state with error:\n") +
				 listQuery.lastError().text());
		return {};
	}

	ChangeHash stateHash;
	while(listQuery.next()) {
		ObjectKey key;
		key.first = listQuery.value(0).toByteArray();
		key.second = listQuery.value(1).toString();
		stateHash.insert(key, (ChangeState)listQuery.value(2).toInt());
	}

	return stateHash;
}

void SqlStateHolder::markLocalChanged(const ObjectKey &key, StateHolder::ChangeState changed)
{
	QSqlQuery updateQuery(database);

	if(changed == Unchanged) {
		updateQuery.prepare(QStringLiteral("DELETE FROM SyncState WHERE Type = ? AND Key = ?"));
		updateQuery.addBindValue(key.first);
		updateQuery.addBindValue(key.second);
	} else {
		updateQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO SyncState (Type, Key, Changed) VALUES(?, ?, ?)"));
		updateQuery.addBindValue(key.first);
		updateQuery.addBindValue(key.second);
		updateQuery.addBindValue((int)changed);
	}

	if(!updateQuery.exec()) {
		logFatal(true,
				 QStringLiteral("Failed to update current state for type \"%1\" and key \"%2\" with error:\n")
				 .arg(QString::fromUtf8(key.first))
				 .arg(key.second) +
				 updateQuery.lastError().text());
	}
}

StateHolder::ChangeHash SqlStateHolder::resetAllChanges(const QList<ObjectKey> &changeKeys)
{
	clearAllChanges();

	if(!database.transaction()) {
		logFatal(false,
				 QStringLiteral("Failed to start database transaction with error:\n") +
				 database.lastError().text());
		return {};
	}

	ChangeHash stateHash;
	foreach (auto key, changeKeys) {
		stateHash.insert(key, Changed);

		QSqlQuery keyQuery(database);
		keyQuery.prepare(QStringLiteral("INSERT INTO SyncState (Type, Key, Changed) VALUES(?, ?, ?)"));
		keyQuery.addBindValue(key.first);
		keyQuery.addBindValue(key.second);
		keyQuery.addBindValue((int)Changed);

		if(!keyQuery.exec()) {
			database.rollback();
			logFatal(true,
					 QStringLiteral("Failed to reset sync state with error:\n") +
					 keyQuery.lastError().text());
			return {};
		}
	}

	if(!database.commit()) {
		logFatal(false,
				 QStringLiteral("Failed to commit database transaction with error:\n") +
				 database.lastError().text());
		return {};
	} else
		return stateHash;
}

void SqlStateHolder::clearAllChanges()
{
	QSqlQuery resetQuery(database);
	resetQuery.prepare(QStringLiteral("DELETE FROM SyncState"));
	if(!resetQuery.exec()) {
		logFatal(true,
				 QStringLiteral("Failed to remove sync state from database with error:\n") +
				 resetQuery.lastError().text());
	}
}
