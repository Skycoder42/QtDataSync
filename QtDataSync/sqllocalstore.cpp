#include "sqllocalstore.h"

#include <QJsonArray>

#include <QtSql>

using namespace QtDataSync;

const QString SqlLocalStore::DatabaseName(QStringLiteral("__QtDataSync_default_database"));

SqlLocalStore::SqlLocalStore(QObject *parent) :
	LocalStore(parent),
	storageDir(),
	database()
{}

void SqlLocalStore::initialize()
{
	storageDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	storageDir.mkpath(QStringLiteral("./qtdatasync_localstore"));
	storageDir.cd(QStringLiteral("./qtdatasync_localstore"));

	database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE3"), DatabaseName);
	database.setDatabaseName(storageDir.absoluteFilePath(QStringLiteral("./store.db")));
	database.open();
}

void SqlLocalStore::finalize()
{
	database.close();
	database = QSqlDatabase();
	QSqlDatabase::removeDatabase(DatabaseName);
}

void SqlLocalStore::loadAll(quint64 id, int metaTypeId)
{
	emit requestCompleted(id, QJsonArray());
}

void SqlLocalStore::load(quint64 id, int metaTypeId, const QString &key, const QString &value)
{
	emit requestCompleted(id, QJsonValue::Null);
}

void SqlLocalStore::save(quint64 id, int metaTypeId, const QString &key, const QJsonObject &object)
{
	//create table
	QSqlQuery createQuery(database);
	createQuery.prepare("CREATE TABLE IF NOT EXISTS ? ("
							"Key	TEXT NOT NULL,"
							"File	TEXT NOT NULL,"
							"PRIMARY KEY(Key)"
						");");
	createQuery.addBindValue(tableName(metaTypeId));
	if(!createQuery.exec()) {
		emit requestFailed(id, createQuery.lastError().text());
		return;
	}

	//save the file

	//save key in database

	emit requestCompleted(id, QJsonValue::Undefined);
}

void SqlLocalStore::remove(quint64 id, int metaTypeId, const QString &key, const QString &value)
{
	emit requestCompleted(id, QJsonValue::Undefined);
}

void SqlLocalStore::removeAll(quint64 id, int metaTypeId)
{
	emit requestCompleted(id, QJsonValue::Undefined);
}

QString SqlLocalStore::tableName(int metaTypeId) const
{
	return QMetaType::typeName(metaTypeId);
}
