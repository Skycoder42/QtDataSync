#include "defaults.h"
#include "sqllocalstore_p.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QUuid>
#include <QtCore/QTemporaryFile>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

using namespace QtDataSync;

#define LOG defaults->loggingCategory()

#define TYPE_DIR(id, typeName) \
	auto tableDir = typeDirectory(id, typeName); \
	if(!tableDir.exists()) \
		return;

#define EXEC_QUERY(query) do {\
	if(!query.exec()) { \
		emit requestFailed(id, QLatin1Char('\n') + query.lastError().text()); \
		return; \
	} \
} while(false)

SqlLocalStore::SqlLocalStore(QObject *parent) :
	LocalStore(parent),
	defaults(nullptr),
	database()
{}

void SqlLocalStore::initialize(Defaults *defaults)
{
	this->defaults = defaults;
	database = defaults->aquireDatabase();

	//create table
	if(!database.tables().contains(QStringLiteral("DataIndex"))) {
		QSqlQuery createQuery(database);
		createQuery.prepare(QStringLiteral("CREATE TABLE DataIndex ("
												"Type	TEXT NOT NULL,"
												"Key	TEXT NOT NULL,"
												"File	TEXT NOT NULL,"
												"PRIMARY KEY(Type, Key)"
										   ");"));
		if(!createQuery.exec()) {
			qCCritical(LOG).noquote() << "Failed to create DataIndex table with error:\n"
									  << createQuery.lastError().text();
		}
	}

	//create index
	QSqlQuery testIndexQuery(database);
	testIndexQuery.prepare(QStringLiteral("SELECT * FROM sqlite_master WHERE name = ?"));
	testIndexQuery.addBindValue(QStringLiteral("index_DataIndex_Type"));
	if(!testIndexQuery.exec()) {
		qCWarning(LOG).noquote() << "Failed to check if index for DataIndex exists with error:\n"
								 << testIndexQuery.lastError().text();
	}
	if(!testIndexQuery.first()) {
		QSqlQuery indexQuery(database);
		indexQuery.prepare(QStringLiteral("CREATE INDEX index_DataIndex_Type ON DataIndex (Type)"));
		if(!indexQuery.exec()) {
			qCCritical(LOG).noquote() << "Failed to create index for DataIndex with error:\n"
									  << indexQuery.lastError().text();
		}
	}
}

void SqlLocalStore::finalize()
{
	database = QSqlDatabase();
	defaults->releaseDatabase();
}

QList<ObjectKey> SqlLocalStore::loadAllKeys()
{
	QSqlQuery loadQuery(database);
	loadQuery.prepare(QStringLiteral("SELECT Type, Key FROM DataIndex"));
	if(!loadQuery.exec()) {
		qCCritical(LOG).noquote() << "Failed to load all existing keys with error:\n"
								  << loadQuery.lastError().text();
		return {};
	}

	QList<ObjectKey> resList;
	while(loadQuery.next())
		resList.append({loadQuery.value(0).toByteArray(), loadQuery.value(1).toString()});

	return resList;
}

void SqlLocalStore::resetStore()
{
	auto storageDir = defaults->storageDir();
	if(storageDir.cd(QStringLiteral("store"))) {
		if(!storageDir.removeRecursively())
			qCWarning(LOG) << "Failed to remove local storage directory!";
	}

	QSqlQuery resetQuery(database);
	resetQuery.prepare(QStringLiteral("DELETE FROM DataIndex"));
	if(!resetQuery.exec()) {
		qCCritical(LOG).noquote() << "Failed to remove data keys from database with error:\n"
								  << resetQuery.lastError().text();
	}
}

void SqlLocalStore::count(quint64 id, const QByteArray &typeName)
{
	QSqlQuery countQuery(database);
	countQuery.prepare(QStringLiteral("SELECT Count(*) FROM DataIndex WHERE Type = ?"));
	countQuery.addBindValue(typeName);
	EXEC_QUERY(countQuery);

	if(countQuery.first())
		emit requestCompleted(id, countQuery.value(0).toInt());
	else
		emit requestCompleted(id, 0);
}

void SqlLocalStore::keys(quint64 id, const QByteArray &typeName)
{
	QSqlQuery keysQuery(database);
	keysQuery.prepare(QStringLiteral("SELECT Key FROM DataIndex WHERE Type = ?"));
	keysQuery.addBindValue(typeName);
	EXEC_QUERY(keysQuery);

	QJsonArray resList;
	while(keysQuery.next())
		resList.append(keysQuery.value(0).toString());

	emit requestCompleted(id, resList);
}

void SqlLocalStore::loadAll(quint64 id, const QByteArray &typeName)
{
	TYPE_DIR(id, typeName)

	QSqlQuery loadQuery(database);
	loadQuery.prepare(QStringLiteral("SELECT File FROM DataIndex WHERE Type = ?"));
	loadQuery.addBindValue(typeName);
	EXEC_QUERY(loadQuery);

	QJsonArray array;
	while(loadQuery.next()) {
		QFile file(tableDir.absoluteFilePath(loadQuery.value(0).toString() + QStringLiteral(".dat")));
		file.open(QIODevice::ReadOnly);
		auto doc = QJsonDocument::fromBinaryData(file.readAll());
		file.close();

		if(doc.isNull() || file.error() != QFile::NoError) {
			emit requestFailed(id, QStringLiteral("Failed to read data from file \"%1\" with error: %2")
							   .arg(file.fileName())
							   .arg(file.errorString()));
			return;
		} else
			array.append(doc.object());
	}

	emit requestCompleted(id, array);
}

void SqlLocalStore::load(quint64 id, const ObjectKey &key, const QByteArray &)
{
	TYPE_DIR(id, key.first)

	QSqlQuery loadQuery(database);
	loadQuery.prepare(QStringLiteral("SELECT File FROM DataIndex WHERE Type = ? AND Key = ?"));
	loadQuery.addBindValue(key.first);
	loadQuery.addBindValue(key.second);
	EXEC_QUERY(loadQuery);

	if(loadQuery.first()) {
		QFile file(tableDir.absoluteFilePath(loadQuery.value(0).toString() + QStringLiteral(".dat")));
		file.open(QIODevice::ReadOnly);
		auto doc = QJsonDocument::fromBinaryData(file.readAll());
		file.close();

		if(doc.isNull() || file.error() != QFile::NoError) {
			emit requestFailed(id, QStringLiteral("Failed to read data from file \"%1\" with error: %2")
							   .arg(file.fileName())
							   .arg(file.errorString()));
		} else
			emit requestCompleted(id, doc.object());
	} else {
		emit requestFailed(id, QStringLiteral("No data entry of type %1 with id %2 exists!")
						   .arg(QString::fromUtf8(key.first))
						   .arg(key.second));
	}
}

void SqlLocalStore::save(quint64 id, const ObjectKey &key, const QJsonObject &object, const QByteArray &)
{
	TYPE_DIR(id, key.first)

	//check if the file exists
	QSqlQuery existQuery(database);
	existQuery.prepare(QStringLiteral("SELECT File FROM DataIndex WHERE Type = ? AND Key = ?"));
	existQuery.addBindValue(key.first);
	existQuery.addBindValue(key.second);
	EXEC_QUERY(existQuery);

	auto needUpdate = false;
	QScopedPointer<QFile> file;
	if(existQuery.first()) {
		file.reset(new QFile(tableDir.absoluteFilePath(existQuery.value(0).toString() + QStringLiteral(".dat"))));
		file->open(QIODevice::WriteOnly);
	} else {
		auto fileName = QString::fromUtf8(QUuid::createUuid().toRfc4122().toHex());
		fileName = tableDir.absoluteFilePath(QStringLiteral("%1XXXXXX.dat")).arg(fileName);
		auto tFile = new QTemporaryFile(fileName);
		tFile->setAutoRemove(false);
		tFile->open();
		file.reset(tFile);
		needUpdate = true;
	}

	//save the file
	QJsonDocument doc(object);
	file->write(doc.toBinaryData());
	file->close();
	if(file->error() != QFile::NoError) {
		emit requestFailed(id, QStringLiteral("Failed to write data to file \"%1\" with error: %2")
						   .arg(file->fileName())
						   .arg(file->errorString()));
		return;
	}

	//save key in database
	if(needUpdate) {
		QSqlQuery insertQuery(database);
		insertQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO DataIndex (Type, Key, File) VALUES(?, ?, ?)"));
		insertQuery.addBindValue(key.first);
		insertQuery.addBindValue(key.second);
		insertQuery.addBindValue(tableDir.relativeFilePath(QFileInfo(file->fileName()).completeBaseName()));
		EXEC_QUERY(insertQuery);
	}

	emit requestCompleted(id, QJsonValue::Undefined);
}

void SqlLocalStore::remove(quint64 id, const ObjectKey &key, const QByteArray &)
{
	TYPE_DIR(id, key.first)

	QSqlQuery loadQuery(database);
	loadQuery.prepare(QStringLiteral("SELECT File FROM DataIndex WHERE Type = ? AND Key = ?"));
	loadQuery.addBindValue(key.first);
	loadQuery.addBindValue(key.second);
	EXEC_QUERY(loadQuery);

	if(loadQuery.first()) {
		auto fileName = tableDir.absoluteFilePath(loadQuery.value(0).toString() + QStringLiteral(".dat"));
		if(!QFile::remove(fileName)) {
			emit requestFailed(id, QStringLiteral("Failed to delete file %1").arg(fileName));
			return;
		}

		QSqlQuery removeQuery(database);
		removeQuery.prepare(QStringLiteral("DELETE FROM DataIndex WHERE Type = ? AND Key = ?"));
		removeQuery.addBindValue(key.first);
		removeQuery.addBindValue(key.second);
		EXEC_QUERY(removeQuery);

		emit requestCompleted(id, true);
	} else
		emit requestCompleted(id, false);
}

void SqlLocalStore::search(quint64 id, const QByteArray &typeName, const QString &searchQuery)
{
	TYPE_DIR(id, typeName)

	auto query = searchQuery;
	query.replace(QLatin1Char('*'), QLatin1Char('%'));
	query.replace(QLatin1Char('?'), QLatin1Char('_'));

	QSqlQuery findQuery(database);
	findQuery.prepare(QStringLiteral("SELECT File FROM DataIndex WHERE Type = ? AND Key LIKE ?"));
	findQuery.addBindValue(typeName);
	findQuery.addBindValue(query);
	EXEC_QUERY(findQuery);

	QJsonArray array;
	while(findQuery.next()) {
		QFile file(tableDir.absoluteFilePath(findQuery.value(0).toString() + QStringLiteral(".dat")));
		file.open(QIODevice::ReadOnly);
		auto doc = QJsonDocument::fromBinaryData(file.readAll());
		file.close();

		if(doc.isNull() || file.error() != QFile::NoError) {
			emit requestFailed(id, QStringLiteral("Failed to read data from file \"%1\" with error: %2")
							   .arg(file.fileName())
							   .arg(file.errorString()));
			return;
		} else
			array.append(doc.object());
	}

	emit requestCompleted(id, array);
}

QDir SqlLocalStore::typeDirectory(quint64 id, const QByteArray &typeName)
{
	auto tName = QString::fromUtf8("store/_" + QByteArray(typeName).toHex());
	auto tableDir = defaults->storageDir();
	if(!tableDir.mkpath(tName) || !tableDir.cd(tName)) {
		emit requestFailed(id, QStringLiteral("Failed to create table directory \"%1\" for type \"%2\"")
						   .arg(tName)
						   .arg(QString::fromUtf8(typeName)));
		return QDir();
	} else
		return tableDir;
}

bool SqlLocalStore::testTableExists(const QString &tableName) const
{
	return database.tables().contains(tableName);
}
