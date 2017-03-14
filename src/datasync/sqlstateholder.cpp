#include "defaults.h"
#include "sqlstateholder_p.h"

#include <QtCore/QDebug>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

using namespace QtDataSync;

#define LOG defaults->loggingCategory()

SqlStateHolder::SqlStateHolder(QObject *parent) :
	StateHolder(parent),
	defaults(nullptr),
	database()
{}

void SqlStateHolder::initialize(Defaults *defaults)
{
	this->defaults = defaults;
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
			qCCritical(LOG) << "Failed to create SyncState table with error:"
							<< createQuery.lastError().text();
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
		qCCritical(LOG) << "Failed to load current state with error:"
						<< listQuery.lastError().text();
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
		qCCritical(LOG) << "Failed to update current state for type"
						<< key.first
						<< "and key"
						<< key.second
						<< "with error:"
						<< updateQuery.lastError().text();
	}
}
