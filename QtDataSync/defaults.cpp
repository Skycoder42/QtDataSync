#include "defaults.h"
#include <QStandardPaths>
#include <QSqlError>
#include <QDebug>
using namespace QtDataSync;

QHash<QString, quint64> Defaults::refCounter;
const QString Defaults::DatabaseName(QStringLiteral("__QtDataSync_default_database"));

QSettings *Defaults::settings(const QDir &storageDir, QObject *parent)
{
	auto path = storageDir.absoluteFilePath(QStringLiteral("config.ini"));
	return new QSettings(path, QSettings::IniFormat, parent);
}

QSqlDatabase Defaults::aquireDatabase(const QDir &storageDir)
{
	auto dirPath = storageDir.canonicalPath();
	if(refCounter[dirPath]++ == 0) {
		auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), DatabaseName);
		database.setDatabaseName(storageDir.absoluteFilePath(QStringLiteral("./store.db")));
		if(!database.open()) {
			qCritical() << "Failed to open database! All subsequent operations will fail! Database error:"
						<< database.lastError().text();
		}
	}

	return QSqlDatabase::database(DatabaseName);
}

void Defaults::releaseDatabase(const QDir &storageDir)
{
	if(--refCounter[storageDir.canonicalPath()] == 0) {
		QSqlDatabase::database(DatabaseName).close();
		QSqlDatabase::removeDatabase(DatabaseName);
	}
}
