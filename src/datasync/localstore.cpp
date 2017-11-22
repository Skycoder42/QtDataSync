#include "localstore_p.h"

#include <QtCore/QUrl>
#include <QtCore/QJsonDocument>
#include <QtCore/QUuid>
#include <QtCore/QTemporaryFile>

#include <QtSql/QSqlQuery>

using namespace QtDataSync;

#define QTDATASYNC_LOG logger

#define EXEC_QUERY(query, key) do {\
	if(!query.exec()) { \
		throw LocalStoreException(defaults, key, database.databaseName(), query.lastError().text()); \
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

	auto table = getTable(typeName);
	if(table.isEmpty())
		return 0;

	QSqlQuery countQuery(database);
	countQuery.prepare(QStringLiteral("SELECT Count(*) FROM %1").arg(table));
	EXEC_QUERY(countQuery, typeName);

	if(countQuery.first())
		return countQuery.value(0).toInt();
	else
		return 0;
}

QStringList LocalStore::keys(const QByteArray &typeName)
{
	QReadLocker _(&globalLock);

	auto table = getTable(typeName);
	if(table.isEmpty())
		return {};

	QSqlQuery keysQuery(database);
	keysQuery.prepare(QStringLiteral("SELECT Key FROM %1").arg(table));
	EXEC_QUERY(keysQuery, typeName);

	QStringList resList;
	while(keysQuery.next())
		resList.append(keysQuery.value(0).toString());
	return resList;
}

QList<QJsonObject> LocalStore::loadAll(const QByteArray &typeName)
{
	QReadLocker _(&globalLock);

	auto table = getTable(typeName);
	if(table.isEmpty())
		return {};

	QSqlQuery loadQuery(database);
	loadQuery.prepare(QStringLiteral("SELECT Key, File FROM %1").arg(table));
	EXEC_QUERY(loadQuery, typeName);

	QList<QJsonObject> array;
	while(loadQuery.next())
		array.append(readJson(table, loadQuery.value(1).toString(), {typeName, loadQuery.value(0).toString()}));
	return array;
}

QJsonObject LocalStore::load(const ObjectKey &key)
{
	QReadLocker _(&globalLock);

	auto table = getTable(key.typeName);
	if(table.isEmpty())
		throw NoDataException(defaults, key);

	QSqlQuery loadQuery(database);
	loadQuery.prepare(QStringLiteral("SELECT File FROM %1 WHERE Key = ?").arg(table));
	loadQuery.addBindValue(key.id);
	EXEC_QUERY(loadQuery, key);

	if(loadQuery.first())
		return readJson(table, loadQuery.value(0).toString(), key);
	else
		throw NoDataException(defaults, key);
}

void LocalStore::save(const ObjectKey &key, const QJsonObject &data)
{
	QWriteLocker _(&globalLock);

	auto table = getTable(key.typeName, true);
	auto tableDir = typeDirectory(table, key);

	//check if the file exists
	QSqlQuery existQuery(database);
	existQuery.prepare(QStringLiteral("SELECT File FROM %1 WHERE Key = ?").arg(table));
	existQuery.addBindValue(key.id);
	EXEC_QUERY(existQuery, key);

	auto needUpdate = false;
	QScopedPointer<QFile> file;
	if(existQuery.first()) {
		file.reset(new QFile(tableDir.absoluteFilePath(existQuery.value(0).toString() + QStringLiteral(".dat"))));
		if(!file->open(QIODevice::WriteOnly))
			throw LocalStoreException(defaults, key, file->fileName(), file->errorString());
	} else {
		auto fileName = QString::fromUtf8(QUuid::createUuid().toRfc4122().toHex());
		fileName = tableDir.absoluteFilePath(QStringLiteral("%1XXXXXX.dat")).arg(fileName);
		auto tFile = new QTemporaryFile(fileName);
		tFile->setAutoRemove(false);
		if(!tFile->open())
			throw LocalStoreException(defaults, key, tFile->fileName(), tFile->errorString());
		file.reset(tFile);
		needUpdate = true;
	}

	//save the file
	QJsonDocument doc(data);
	file->write(doc.toBinaryData());
	file->close();
	if(file->error() != QFile::NoError)
		throw LocalStoreException(defaults, key, file->fileName(), file->errorString());

	//save key in database
	if(needUpdate) {
		QSqlQuery insertQuery(database);
		insertQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO %1 (Key, File) VALUES(?, ?)").arg(table));
		insertQuery.addBindValue(key.id);
		insertQuery.addBindValue(tableDir.relativeFilePath(QFileInfo(file->fileName()).completeBaseName()));
		EXEC_QUERY(insertQuery, key);
	}
}

bool LocalStore::remove(const ObjectKey &key)
{
	QWriteLocker _(&globalLock);

	auto table = getTable(key.typeName);
	if(table.isEmpty()) //no table -> nothing to delete
		return false;

	QSqlQuery loadQuery(database);
	loadQuery.prepare(QStringLiteral("SELECT File FROM %1 WHERE Key = ?").arg(table));
	loadQuery.addBindValue(key.id);
	EXEC_QUERY(loadQuery, key);

	if(loadQuery.first()) {
		auto tableDir = typeDirectory(table, key);
		auto fileName = tableDir.absoluteFilePath(loadQuery.value(0).toString() + QStringLiteral(".dat"));
		if(!QFile::remove(fileName))
			throw LocalStoreException(defaults, key, fileName, QStringLiteral("Failed to delete file"));

		QSqlQuery removeQuery(database);
		removeQuery.prepare(QStringLiteral("DELETE FROM %1 WHERE Key = ?").arg(table));
		removeQuery.addBindValue(key.id);
		EXEC_QUERY(removeQuery, key);

		return true;
	} else
		return false;
}

QList<QJsonObject> LocalStore::find(const QByteArray &typeName, const QString &query)
{
	QReadLocker _(&globalLock);

	auto table = getTable(typeName);
	if(table.isEmpty())
		return {};

	auto searchQuery = query;
	searchQuery.replace(QLatin1Char('*'), QLatin1Char('%'));
	searchQuery.replace(QLatin1Char('?'), QLatin1Char('_'));

	QSqlQuery findQuery(database);
	findQuery.prepare(QStringLiteral("SELECT Key, File FROM %1 WHERE Key LIKE ?").arg(table));
	findQuery.addBindValue(searchQuery);
	EXEC_QUERY(findQuery, typeName);

	QList<QJsonObject> array;
	while(findQuery.next())
		array.append(readJson(table, findQuery.value(1).toString(), {typeName, findQuery.value(0).toString()}));
	return array;
}

QString LocalStore::getTable(const QByteArray &typeName, bool allowCreate)
{
	//create table
	if(!tableNameCache.contains(typeName)) {
		auto encName = QUrl::toPercentEncoding(QString::fromUtf8(typeName))
					   .replace('%', '_');
		auto tableName = QStringLiteral("data_%1")
						 .arg(QString::fromUtf8(encName));

		if(!database.tables().contains(tableName)) {
			if(allowCreate) {
				QSqlQuery createQuery(database);
				createQuery.prepare(QStringLiteral("CREATE TABLE %1 ("
												   "Key		TEXT NOT NULL,"
												   "Version	INTEGER NOT NULL,"
												   "File		TEXT NOT NULL,"
												   "Checksum	BLOB NOT NULL"
												   "PRIMARY KEY(Key)"
												   ");").arg(tableName));
				if(!createQuery.exec())
					throw LocalStoreException(defaults, typeName, database.databaseName(), createQuery.lastError().text());

				tableNameCache.insert(typeName, tableName);
			}
		} else
			tableNameCache.insert(typeName, tableName);
	}

	return tableNameCache.value(typeName);
}

QDir LocalStore::typeDirectory(const QString &tableName, const ObjectKey &key)
{
	auto tName = QStringLiteral("store/%1").arg(tableName);
	auto tableDir = defaults.storageDir();
	if(!tableDir.mkpath(tName) || !tableDir.cd(tName)) {
		throw LocalStoreException(defaults, key, tName, QStringLiteral("Failed to create directory"));
	} else
		return tableDir;
}

QJsonObject LocalStore::readJson(const QString &tableName, const QString &fileName, const ObjectKey &key)
{
	QFile file(typeDirectory(tableName, key).absoluteFilePath(fileName + QStringLiteral(".dat")));
	if(!file.open(QIODevice::ReadOnly))
		throw LocalStoreException(defaults, key, file.fileName(), file.errorString());
	auto doc = QJsonDocument::fromBinaryData(file.readAll());
	file.close();

	if(!doc.isObject()) {
		throw LocalStoreException(defaults, key, file.fileName(), QStringLiteral("File contains invalid json data"));
	}

	return doc.object();
}

// ------------- Exceptions -------------

LocalStoreException::LocalStoreException(const Defaults &defaults, const ObjectKey &key, const QString &context, const QString &message) :
	Exception(defaults, message),
	_key(key),
	_context(context)
{}

LocalStoreException::LocalStoreException(const LocalStoreException * const other) :
	Exception(other),
	_key(other->_key),
	_context(other->_context)
{}

ObjectKey LocalStoreException::key() const
{
	return _key;
}

QString LocalStoreException::context() const
{
	return _context;
}

QString LocalStoreException::qWhat() const
{
	QString msg = Exception::qWhat() +
				  QStringLiteral("\n\tContext: %1"
								 "\n\tTypeName: %2")
				  .arg(_context)
				  .arg(QString::fromUtf8(_key.typeName));
	if(!_key.id.isNull())
		msg += QStringLiteral("\n\tKey: %1").arg(_key.id);
	return msg;
}

void LocalStoreException::raise() const
{
	throw (*this);
}

QException *LocalStoreException::clone() const
{
	return new LocalStoreException(this);
}



NoDataException::NoDataException(const Defaults &defaults, const ObjectKey &key) :
	Exception(defaults, QStringLiteral("The requested data does not exist")),
	_key(key)
{}

NoDataException::NoDataException(const NoDataException * const other) :
	Exception(other),
	_key(other->_key)
{}

ObjectKey NoDataException::key() const
{
	return _key;
}

QString NoDataException::qWhat() const
{
	return Exception::qWhat() +
			QStringLiteral("\n\tTypeName: %1"
						   "\n\tKey: %2")
			.arg(QString::fromUtf8(_key.typeName))
			.arg(_key.id);
}

void NoDataException::raise() const
{
	throw (*this);
}

QException *NoDataException::clone() const
{
	return new NoDataException(this);
}
