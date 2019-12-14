#include "localstore_p.h"
#include "engine_p.h"
#include <QtCore/QEvent>
using namespace QtDataSync;

const QString LocalStore::IndexTable = QStringLiteral("StorageIndex");

LocalStore::LocalStore(Engine *engine, QObject *parent) :
	QObject{parent},
	_database{engine, this}
{
	createTables();
}

quint64 LocalStore::count(const QByteArray &typeName) const
{
	QSqlQuery countQuery{_database};
	countQuery.prepare(QStringLiteral("SELECT COUNT(*) "
									  "FROM %1 "
									  "WHERE type == ?")
						   .arg(IndexTable));
	countQuery.addBindValue(typeName);
	if (!countQuery.exec())
		throw LocalStoreException{countQuery};
	if (countQuery.first())
		return countQuery.value(0).toULongLong();
	else
		return 0;
}

QStringList LocalStore::keys(const QByteArray &typeName) const
{

}

void LocalStore::createTables()
{
	if (!_database->tables().contains(IndexTable)) {
		QSqlQuery createQuery(_database);
		if (!createQuery.exec(QStringLiteral(R"__(CREATE TABLE "%1" (
			"type"		TEXT NOT NULL,
			"id"		TEXT NOT NULL,
			"data"		BLOB,
			"version"	INTEGER NOT NULL DEFAULT 0,
			"changed"	INTEGER NOT NULL DEFAULT 1 CHECK(changed == 0 OR changed == 1),
			PRIMARY KEY("type","id")
		) WITHOUT ROWID;)__").arg(IndexTable)))
			throw LocalStoreException{createQuery};
		// TODO debug success
	}
}



LocalStoreException::LocalStoreException(const QSqlError &error) :
	_error{error.text()}
{}

LocalStoreException::LocalStoreException(const QSqlDatabase &database) :
	LocalStoreException{database.lastError()}
{}

LocalStoreException::LocalStoreException(const QSqlQuery &query) :
	LocalStoreException{query.lastError()}
{}

QString LocalStoreException::qWhat() const
{
	return _error;
}

void LocalStoreException::raise() const
{
	throw *this;
}

ExceptionBase *LocalStoreException::clone() const
{
	return new LocalStoreException{*this};
}


QThreadStorage<LocalDatabase::DatabaseInfo> LocalDatabase::_databaseIds;

LocalDatabase::LocalDatabase(Engine *engine, QObject *parent) :
	QObject{parent},
	_engine{engine}
{
	acquireDb();
	parent->installEventFilter(this);
}

LocalDatabase::~LocalDatabase()
{
	parent()->removeEventFilter(this);
	releaseDb();
}

QtDataSync::LocalDatabase::operator QSqlDatabase() const
{
	acquireDb();
	return *_database;
}

QSqlDatabase *LocalDatabase::operator->() const
{
	acquireDb();
	return &(*_database);
}

bool LocalDatabase::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::ThreadChange && watched == parent())
		releaseDb();
	return false;
}

void LocalDatabase::acquireDb() const
{
	if (!_database) {
		if (!_databaseIds.hasLocalData()) {
			const auto setup = EnginePrivate::setupFor(_engine);
			const auto dbId = QUuid::createUuid();
			_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), dbId.toString(QUuid::WithoutBraces));
			if (!_database->isValid())
				throw LocalStoreException{*_database};
			_database->setDatabaseName(setup->localDir->absoluteFilePath(QStringLiteral("local-storage.db")));
			_database->setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=30000;"
														"QSQLITE_ENABLE_REGEXP"));
			if (!_database->open())
				throw LocalStoreException{*_database};
			_databaseIds.setLocalData(std::make_pair(QUuid::createUuid(), 1));
		} else {
			auto &dbInfo = _databaseIds.localData();
			_database = QSqlDatabase::database(dbInfo.first.toString(QUuid::WithoutBraces));
			if (dbInfo.second == 0 && !_database->isOpen()) {
				if (!_database->open())
					throw LocalStoreException{*_database};
			}
			++dbInfo.second;
		}
	}
}

void LocalDatabase::releaseDb() const
{
	if (_database) {
		auto &dbInfo = _databaseIds.localData();
		if (--dbInfo.second == 0 && _database->isOpen())
			_database->close();
		_database = std::nullopt;
	}
}
