#include "defaults.h"
#include "defaults_p.h"
#include "setup.h"

#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>

#include <QtSql/QSqlError>

using namespace QtDataSync;

#define LOG d->logCat

const QString DefaultsPrivate::DatabaseName(QStringLiteral("__QtDataSync_default_database"));

Defaults::Defaults(const QString &setupName, const QDir &storageDir, QObject *parent) :
	QObject(parent),
	d(new DefaultsPrivate(setupName, storageDir))
{
	d->settings = createSettings(this);
}

Defaults::~Defaults()
{
	if(d->dbRefCounter != 0)
		qCWarning(LOG) << "Number of database references is not 0!";
}

const QLoggingCategory &Defaults::loggingCategory() const
{
	return d->logCat;
}

QDir Defaults::storageDir() const
{
	return d->storageDir;
}

QSettings *Defaults::settings() const
{
	return d->settings;
}

QSettings *Defaults::createSettings(QObject *parent) const
{
	auto path = d->storageDir.absoluteFilePath(QStringLiteral("config.ini"));
	return new QSettings(path, QSettings::IniFormat, parent);
}

QSqlDatabase Defaults::aquireDatabase()
{
	if(d->dbRefCounter++ == 0) {
		auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), DefaultsPrivate::DatabaseName);
		database.setDatabaseName(d->storageDir.absoluteFilePath(QStringLiteral("./store.db")));
		if(!database.open()) {
			qCCritical(LOG) << "Failed to open database! All subsequent operations will fail! Database error:"
							<< database.lastError().text();
		}
	}

	return QSqlDatabase::database(DefaultsPrivate::DatabaseName);
}

void Defaults::releaseDatabase()
{
	if(--d->dbRefCounter == 0) {
		QSqlDatabase::database(DefaultsPrivate::DatabaseName).close();
		QSqlDatabase::removeDatabase(DefaultsPrivate::DatabaseName);
	}
}



QtDataSync::DefaultsPrivate::DefaultsPrivate(const QString &setupName, const QDir &storageDir) :
	storageDir(storageDir),
	dbRefCounter(0),
	catName("QtDataSync:" + setupName.toUtf8()),
	logCat(catName),
	settings(nullptr)
{

}
