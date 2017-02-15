#include "defaults.h"
#include "setup.h"
#include <QStandardPaths>
#include <QSqlError>
#include <QDebug>
using namespace QtDataSync;

#define LOG Defaults::loggingCategory(storageDir)

QHash<QString, quint64> Defaults::refCounter;
const QString Defaults::DatabaseName(QStringLiteral("__QtDataSync_default_database"));
QHash<QString, QPair<QByteArray, QSharedPointer<QLoggingCategory>>> Defaults::sNames;

void Defaults::registerSetup(const QDir &storageDir, const QString &name)
{
	auto bName = "QtDataSync:" + name.toUtf8();
	sNames.insert(storageDir.canonicalPath(), {
					  bName,
					  QSharedPointer<QLoggingCategory>::create(bName)
				  });
}

void Defaults::unregisterSetup(const QDir &storageDir)
{
	sNames.remove(storageDir.canonicalPath());
}

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
			qCCritical(LOG) << "Failed to open database! All subsequent operations will fail! Database error:"
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

const QLoggingCategory &Defaults::loggingCategory(const QDir &storageDir)
{
	auto catPtr = sNames.value(storageDir.canonicalPath());
	if(catPtr.second)
		return *(catPtr.second);
	else {
		static const QLoggingCategory cCat("QtDataSync_unkown");
		return cCat;
	}
}
