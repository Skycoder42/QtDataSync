#include "defaultsqlkeeper.h"
#include <QStandardPaths>
#include <QSqlError>
#include <QDebug>
using namespace QtDataSync;

QHash<QString, quint64> DefaultSqlKeeper::refCounter;
const QString DefaultSqlKeeper::DatabaseName(QStringLiteral("__QtDataSync_default_database"));

QDir DefaultSqlKeeper::storageDir(const QString &localDir)
{
	QDir storageDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	storageDir.mkpath(localDir);
	storageDir.cd(localDir);
	return storageDir;
}

QSqlDatabase DefaultSqlKeeper::aquireDatabase(const QString &localDir)
{
	if(refCounter[localDir]++ == 0) {
		auto dir = storageDir(localDir);
		auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), DatabaseName);
		database.setDatabaseName(dir.absoluteFilePath(QStringLiteral("./store.db")));
		if(!database.open()) {
			qCritical() << "Failed to open database! All subsequent operations will fail! Database error:"
						<< database.lastError().text();
		}
	}

	return QSqlDatabase::database(DatabaseName);
}

void DefaultSqlKeeper::releaseDatabase(const QString &localDir)
{
	if(--refCounter[localDir] == 0) {
		QSqlDatabase::database(DatabaseName).close();
		QSqlDatabase::removeDatabase(DatabaseName);
	}
}
