#include "defaultsqlkeeper.h"
#include <QStandardPaths>
#include <QSqlError>
#include <QDebug>
using namespace QtDataSync;

quint64 DefaultSqlKeeper::refCounter = 0;
const QString DefaultSqlKeeper::DatabaseName(QStringLiteral("__QtDataSync_default_database"));

QDir DefaultSqlKeeper::storageDir()
{
	QDir storageDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	storageDir.mkpath(QStringLiteral("./qtdatasync_localstore"));
	storageDir.cd(QStringLiteral("./qtdatasync_localstore"));
	return storageDir;
}

QSqlDatabase DefaultSqlKeeper::aquireDatabase()
{
	if(refCounter++ == 0) {
		auto dir = storageDir();
		auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), DatabaseName);
		database.setDatabaseName(dir.absoluteFilePath(QStringLiteral("./store.db")));
		if(!database.open()) {
			qCritical() << "Failed to open database! All subsequent operations will fail! Database error:"
						<< database.lastError().text();
		}
	}

	return QSqlDatabase::database(DatabaseName);
}

void DefaultSqlKeeper::releaseDatabase()
{
	if(--refCounter == 0) {
		QSqlDatabase::database(DatabaseName).close();
		QSqlDatabase::removeDatabase(DatabaseName);
	}
}
