#include "defaults.h"
#include "defaults_p.h"

#include <QtCore/QThread>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>

#include <QtSql/QSqlError>


using namespace QtDataSync;

#define QTDATASYNC_LOG d->logger

const QString DefaultsPrivate::DatabaseName(QStringLiteral("__QtDataSync_default_database"));

Defaults::Defaults(const QString &setupName, const QDir &storageDir, const QHash<QByteArray, QVariant> &properties, const QJsonSerializer *serializer) :
	d(new DefaultsPrivate(setupName, storageDir, serializer))
{
	for(auto it = properties.constBegin(); it != properties.constEnd(); it++)
		d->setProperty(it.key().constData(), it.value());
	d->settings = createSettings(d.data());
}

Defaults::Defaults(const Defaults &other) :
	d(other.d)
{}

Logger *Defaults::createLogger(const QByteArray &subCategory, QObject *parent) const
{
	return new Logger(subCategory, d->setupName, parent);
}

QString Defaults::setupName() const
{
	return d->setupName;
}

QDir Defaults::storageDir() const
{
	return d->storageDir;
}

QSettings *Defaults::settings() const
{
	Q_ASSERT_X(d->settings->thread() == QThread::currentThread(), Q_FUNC_INFO, "You can only use the default settings from the engine thread");
	return d->settings;
}

QSettings *Defaults::createSettings(QObject *parent) const
{
	auto path = d->storageDir.absoluteFilePath(QStringLiteral("config.ini"));
	return new QSettings(path, QSettings::IniFormat, parent);
}

const QJsonSerializer *Defaults::serializer() const
{
	return d->serializer;
}

QSqlDatabase Defaults::aquireDatabase()
{
	if(d->dbRefCounter++ == 0) {
		auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), DefaultsPrivate::DatabaseName);
		database.setDatabaseName(d->storageDir.absoluteFilePath(QStringLiteral("./store.db")));
		if(!database.open()) {
			logFatal(false,
					 QStringLiteral("Failed to open database! All subsequent operations will fail! Database error:\n") +
					 database.lastError().text());
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



#undef QTDATASYNC_LOG
#define QTDATASYNC_LOG logger

QtDataSync::DefaultsPrivate::DefaultsPrivate(const QString &setupName, const QDir &storageDir, const QJsonSerializer *serializer) :
	setupName(setupName),
	storageDir(storageDir),
	dbRefCounter(0),
	logger(new Logger("defaults", setupName, this)),
	settings(nullptr),
	serializer(serializer)
{}

DefaultsPrivate::~DefaultsPrivate()
{
	if(dbRefCounter != 0)
		logWarning() << "Number of database references is not 0 on destruction!";
}
