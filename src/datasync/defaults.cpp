#include "defaults.h"
#include "defaults_p.h"

#include <QtCore/QThread>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>

#include <QtSql/QSqlError>


using namespace QtDataSync;

#define QTDATASYNC_LOG d->logger

const QString DefaultsPrivate::DatabaseName(QStringLiteral("__QtDataSync_database_%1"));

Defaults::Defaults(const QString &setupName) :
	d(DefaultsPrivate::obtainDefaults(setupName))
{
	Q_ASSERT_X(d, Q_FUNC_INFO, "You can't obtain non-existant defaults");//TODO exception
}

Defaults::Defaults(const Defaults &other) :
	d(other.d)
{}

Defaults::~Defaults() {}

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

QSettings *Defaults::createSettings(QObject *parent) const
{
	auto path = d->storageDir.absoluteFilePath(QStringLiteral("config.ini"));
	return new QSettings(path, QSettings::IniFormat, parent);
}

const QJsonSerializer *Defaults::serializer() const
{
	return d->serializer;
}

DatabaseRef Defaults::aquireDatabase(QSqlDatabase &dbMember)
{
	return DatabaseRef(new DatabaseRefPrivate(d, dbMember));
}




DatabaseRef::DatabaseRef() :
	d(nullptr)
{}

DatabaseRef::~DatabaseRef() {}

DatabaseRef::DatabaseRef(DatabaseRefPrivate *d) :
	d(d)
{}

DatabaseRef::DatabaseRef(DatabaseRef &&other) :
	d(nullptr)
{
	d.swap(other.d);
}

DatabaseRef &DatabaseRef::operator =(DatabaseRef &&other)
{
	d.reset();
	d.swap(other.d);
	return (*this);
}




#undef QTDATASYNC_LOG
#define QTDATASYNC_LOG logger

QMutex DefaultsPrivate::setupDefaultsMutex;
QHash<QString, QSharedPointer<DefaultsPrivate>> DefaultsPrivate::setupDefaults;

void DefaultsPrivate::createDefaults(const QString &setupName, const QDir &storageDir, const QHash<QByteArray, QVariant> &properties, QJsonSerializer *serializer)
{
	QMutexLocker _(&setupDefaultsMutex);
	setupDefaults.insert(setupName, QSharedPointer<DefaultsPrivate>::create(setupName, storageDir, properties, serializer));
}

void DefaultsPrivate::removeDefaults(const QString &setupName)
{
	QMutexLocker _(&setupDefaultsMutex);
	setupDefaults.remove(setupName);
}

void DefaultsPrivate::clearDefaults()
{
	QMutexLocker _(&setupDefaultsMutex);
	setupDefaults.clear();
}

QSharedPointer<DefaultsPrivate> DefaultsPrivate::obtainDefaults(const QString &setupName)
{
	QMutexLocker _(&setupDefaultsMutex);
	return setupDefaults.value(setupName);
}

QtDataSync::DefaultsPrivate::DefaultsPrivate(const QString &setupName, const QDir &storageDir, const QHash<QByteArray, QVariant> &properties, QJsonSerializer *serializer) :
	setupName(setupName),
	storageDir(storageDir),
	logger(new Logger("defaults", setupName, this)),
	serializer(serializer),
	dbRefCounter(0)
{
	serializer->setParent(this);
	for(auto it = properties.constBegin(); it != properties.constEnd(); it++)
		setProperty(it.key().constData(), it.value());
}

DefaultsPrivate::~DefaultsPrivate()
{
	if(dbRefCounter != 0)
		logWarning() << "Number of database references is not 0 on destruction!";
}

QSqlDatabase DefaultsPrivate::acquireDatabase()
{
	auto name = DefaultsPrivate::DatabaseName.arg(setupName);
	if(dbRefCounter++ == 0) {
		auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
		database.setDatabaseName(storageDir.absoluteFilePath(QStringLiteral("store.db")));
		if(!database.open()) {
			logFatal(false,
					 QStringLiteral("Failed to open database! All subsequent operations will fail! Database error:\n") +
					 database.lastError().text());
		}
	}

	return QSqlDatabase::database(name);
}

void DefaultsPrivate::releaseDatabase()
{
	if(--dbRefCounter == 0) {
		auto name = DefaultsPrivate::DatabaseName.arg(setupName);
		QSqlDatabase::database(name).close();
		QSqlDatabase::removeDatabase(name);
	}
}



DatabaseRefPrivate::DatabaseRefPrivate(QSharedPointer<DefaultsPrivate> defaultsPrivate, QSqlDatabase &memberDb) :
	defaultsPrivate(defaultsPrivate),
	memberDb(memberDb)
{
	memberDb = defaultsPrivate->acquireDatabase();
}

DatabaseRefPrivate::~DatabaseRefPrivate()
{
	memberDb = QSqlDatabase();
	defaultsPrivate->releaseDatabase();
}
