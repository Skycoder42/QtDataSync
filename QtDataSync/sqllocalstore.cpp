#include "sqllocalstore.h"

#include <QJsonArray>

#include <QtSql>

using namespace QtDataSync;

#define EXEC_QUERY(query) do {\
	if(!query.exec()) { \
		emit requestFailed(id, query.lastError().text()); \
		return; \
	} \
} while(false)

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

	database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), DatabaseName);
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

void SqlLocalStore::load(quint64 id, int metaTypeId, const QString &, const QString &value)
{
	auto tName = tableName(metaTypeId);

	//check if table exists
	if(testTableExists(tName)) {
		QSqlQuery idQuery(database);
		idQuery.prepare(QStringLiteral("SELECT File FROM %1 WHERE Key = ?").arg(tName));
		idQuery.addBindValue(value);
		EXEC_QUERY(idQuery);

		if(idQuery.first()) {
			QFile file(storageDir.absoluteFilePath(idQuery.value(0).toString() + QStringLiteral(".dat")));
			file.open(QIODevice::ReadOnly);
			auto doc = QJsonDocument::fromBinaryData(file.readAll());
			file.close();

			if(doc.isNull())
				emit requestFailed(id, QStringLiteral("Failed to read data from file %1").arg(file.fileName()));
			else
				emit requestCompleted(id, doc.object());
			return;
		}
	}

	emit requestCompleted(id, QJsonValue::Null);
}

void SqlLocalStore::save(quint64 id, int metaTypeId, const QString &key, const QJsonObject &object)
{
	auto tName = tableName(metaTypeId);
	auto tKey = object[key].toVariant().toString();

	//create table
	QSqlQuery createQuery(database);
	createQuery.prepare(QStringLiteral("CREATE TABLE IF NOT EXISTS %1 ("
							"Key	TEXT NOT NULL,"
							"File	TEXT NOT NULL,"
							"PRIMARY KEY(Key)"
						");").arg(tName));
	EXEC_QUERY(createQuery);

	//check if the file exists
	QSqlQuery existQuery(database);
	existQuery.prepare(QStringLiteral("SELECT File FROM %1 WHERE Key = ?").arg(tName));
	existQuery.addBindValue(tKey);
	EXEC_QUERY(existQuery);

	auto needUpdate = false;
	QFile *file = nullptr;
	if(existQuery.first()) {
		file = new QFile(storageDir.absoluteFilePath(existQuery.value(0).toString() + QStringLiteral(".dat")));
		file->open(QIODevice::WriteOnly);
	} else {
		auto fileName = QString::fromLatin1(QUuid::createUuid().toRfc4122().toHex());
		fileName = storageDir.absoluteFilePath(QStringLiteral("%1XXXXXX.dat")).arg(fileName);
		auto tFile = new QTemporaryFile(fileName);
		tFile->setAutoRemove(false);
		tFile->open();
		file = tFile;
		needUpdate = true;
	}

	//save the file
	QJsonDocument doc(object);
	file->write(doc.toBinaryData());
	file->close();

	//save key in database
	if(needUpdate) {
		QSqlQuery insertQuery(database);
		insertQuery.prepare(QStringLiteral("INSERT INTO %1 (Key, File) VALUES(?, ?)").arg(tName));
		insertQuery.addBindValue(tKey);
		insertQuery.addBindValue(storageDir.relativeFilePath(QFileInfo(file->fileName()).completeBaseName()));
		EXEC_QUERY(insertQuery);
	}

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
	return QString::fromLatin1(QByteArray(QMetaType::typeName(metaTypeId)).toBase64());
}

bool SqlLocalStore::testTableExists(const QString &tableName) const
{
	return database.tables().contains(tableName);
}
