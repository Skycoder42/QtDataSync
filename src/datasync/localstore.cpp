#include "localstore_p.h"
#include "exceptions.h"

#include <QtCore/QUrl>
#include <QtCore/QJsonDocument>

#include <QtSql/QSqlQuery>

using namespace QtDataSync;

#define QTDATASYNC_LOG logger
//TODO proper invalid data exception
#define EXEC_QUERY(query) do {\
	if(!query.exec()) { \
		throw InvalidDataException(query.lastError().text()); \
	} \
} while(false)

QReadWriteLock LocalStore::globalLock;

LocalStore::LocalStore(QObject *parent) :
	LocalStore(DefaultSetup, parent)
{}

LocalStore::LocalStore(const QString &setupName, QObject *parent) :
	QObject(parent),
	defaults(setupName),
	logger(defaults.createLogger(staticMetaObject.className(), this)),
	database(),
	dbRef(defaults.aquireDatabase(database)),
	tableNameCache(),
	dataCache()
{}

quint64 LocalStore::count(const QByteArray &typeName)
{
	QReadLocker _(&globalLock);

	QSqlQuery countQuery(database);
	countQuery.prepare(QStringLiteral("SELECT Count(*) FROM %1").arg(table(typeName)));
	EXEC_QUERY(countQuery);
}

QStringList LocalStore::keys(const QByteArray &typeName)
{
	QReadLocker _(&globalLock);

	QSqlQuery keysQuery(database);
	keysQuery.prepare(QStringLiteral("SELECT Key FROM %1").arg(table(typeName)));
	EXEC_QUERY(keysQuery);

	QStringList resList;
	while(keysQuery.next())
		resList.append(keysQuery.value(0).toString());
	return resList;
}

QList<QJsonObject> LocalStore::loadAll(const QByteArray &typeName)
{
	QReadLocker _(&globalLock);

	QSqlQuery loadQuery(database);
	loadQuery.prepare(QStringLiteral("SELECT File FROM %1").arg(table(typeName)));
	EXEC_QUERY(loadQuery);

	QList<QJsonObject> array;
	while(loadQuery.next())
		array.append(readJson(typeName, loadQuery.value(0).toString()));
	return array;
}

QJsonObject LocalStore::load(const ObjectKey &id)
{
	QReadLocker _(&globalLock);

	QSqlQuery loadQuery(database);
	loadQuery.prepare(QStringLiteral("SELECT File FROM %1 WHERE Key = ?").arg(table(id.typeName)));
	loadQuery.addBindValue(id.id);
	EXEC_QUERY(loadQuery);

	if(loadQuery.first())
		return readJson(id.typeName, loadQuery.value(0).toString());
	else {
		throw InvalidDataException(QStringLiteral("No data entry of type %1 with id %2 exists!")
								   .arg(QString::fromUtf8(id.typeName))
								   .arg(id.id));
	}
}

void LocalStore::save(const ObjectKey &id, const QJsonObject &data)
{

}

void LocalStore::remove(const ObjectKey &id)
{

}

QList<QJsonObject> LocalStore::find(const QByteArray &typeName, const QString &query)
{

}

QString LocalStore::table(const QByteArray &typeName)
{
	//create table
	if(!tableNameCache.contains(typeName)) {
		auto encName = QUrl::toPercentEncoding(QString::fromUtf8(typeName))
					   .replace('%', '_');
		auto tableName = QStringLiteral("data_%1")
						 .arg(QString::fromUtf8(encName));

		if(!database.tables().contains(tableName)) {
			QSqlQuery createQuery(database);
			createQuery.prepare(QStringLiteral("CREATE TABLE %1 ("
													"Key		TEXT NOT NULL,"
													"Version	INTEGER NOT NULL,"
													"File		TEXT NOT NULL,"
													"Checksum	BLOB NOT NULL"
													"PRIMARY KEY(Key)"
											   ");").arg(tableName));
			if(!createQuery.exec()) {
				logFatal(false,
						 QStringLiteral("Failed to create DataIndex table with error:\n") +
						 createQuery.lastError().text());
			}
		}
		tableNameCache.insert(typeName, tableName);
	}

	return tableNameCache.value(typeName);
}

QDir LocalStore::typeDirectory(const QByteArray &typeName)
{
	auto tName = QStringLiteral("store/%1").arg(table(typeName));
	auto tableDir = defaults.storageDir();
	if(!tableDir.mkpath(tName) || !tableDir.cd(tName)) {
		throw InvalidDataException(QStringLiteral("Failed to create table directory \"%1\" for type \"%2\"")
								   .arg(tName)
								   .arg(QString::fromUtf8(typeName)));
	} else
		return tableDir;
}

QJsonObject LocalStore::readJson(const QByteArray &typeName, const QString &fileName)
{
	QFile file(typeDirectory(typeName).absoluteFilePath(fileName + QStringLiteral(".dat")));
	if(!file.open(QIODevice::ReadOnly)) {
		throw InvalidDataException(QStringLiteral("Failed to read data from file \"%1\" with error: %2")
								   .arg(file.fileName())
								   .arg(file.errorString()));
	}
	auto doc = QJsonDocument::fromBinaryData(file.readAll());
	file.close();

	if(!doc.isObject()) {
		throw InvalidDataException(QStringLiteral("File \"%1\" contains invalid json data")
								   .arg(file.fileName()));
	}

	return doc.object();
}
