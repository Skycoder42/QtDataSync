#include "defaults.h"
#include "defaults_p.h"

#include <QtCore/QThread>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>
#include <QtCore/QEvent>

#include <QtSql/QSqlError>

using namespace QtDataSync;

#define QTDATASYNC_LOG d->logger

Defaults::Defaults(const QString &setupName) :
	d(DefaultsPrivate::obtainDefaults(setupName))
{
	if(!d)
		throw SetupDoesNotExistException(setupName);
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

QVariant Defaults::property(Defaults::PropertyKey key) const
{
	return d->properties.value(key);
}

DatabaseRef Defaults::aquireDatabase(QObject *object)
{
	return DatabaseRef(new DatabaseRefPrivate(d, object));
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

QSqlDatabase DatabaseRef::database() const
{
	return d->db();
}

DatabaseRef::operator QSqlDatabase() const
{
	return d->db();
}

QSqlDatabase *DatabaseRef::operator ->() const
{
	return &(d->db());
}




#undef QTDATASYNC_LOG
#define QTDATASYNC_LOG logger

const QString DefaultsPrivate::DatabaseName(QStringLiteral("__QtDataSync_database_%1_0x%2"));
QMutex DefaultsPrivate::setupDefaultsMutex;
QHash<QString, QSharedPointer<DefaultsPrivate>> DefaultsPrivate::setupDefaults;

void DefaultsPrivate::createDefaults(const QString &setupName, const QDir &storageDir, const QHash<Defaults::PropertyKey, QVariant> &properties, QJsonSerializer *serializer)
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

DefaultsPrivate::DefaultsPrivate(const QString &setupName, const QDir &storageDir, const QHash<Defaults::PropertyKey, QVariant> &properties, QJsonSerializer *serializer) :
	setupName(setupName),
	storageDir(storageDir),
	logger(new Logger("defaults", setupName, this)),
	serializer(serializer),
	properties(properties),
	dbRefCounter()
{
	serializer->setParent(this);
}

QSqlDatabase DefaultsPrivate::acquireDatabase(QThread *thread)
{
	auto name = DefaultsPrivate::DatabaseName
				.arg(setupName)
				.arg(QString::number((quint64)thread, 16));
	if(dbRefCounter.localData()++ == 0) {
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

void DefaultsPrivate::releaseDatabase(QThread *thread)
{
	if(--dbRefCounter.localData() == 0) {
		auto name = DefaultsPrivate::DatabaseName
					.arg(setupName)
					.arg(QString::number((quint64)thread, 16));
		QSqlDatabase::database(name).close();
		QSqlDatabase::removeDatabase(name);
	}
}



DatabaseRefPrivate::DatabaseRefPrivate(QSharedPointer<DefaultsPrivate> defaultsPrivate, QObject *object) :
	defaultsPrivate(defaultsPrivate),
	object(object),
	database()
{
	object->installEventFilter(this);
}

DatabaseRefPrivate::~DatabaseRefPrivate()
{
	if(database.isValid()) {
		database = QSqlDatabase();
		defaultsPrivate->releaseDatabase(QThread::currentThread());
	}
}

QSqlDatabase &DatabaseRefPrivate::db()
{
	if(!database.isValid())
		database = defaultsPrivate->acquireDatabase(QThread::currentThread());
	return database;
}

bool DatabaseRefPrivate::eventFilter(QObject *watched, QEvent *event)
{
	if(event->type() == QEvent::ThreadChange && watched == object) {
		if(database.isValid()) {
			database = QSqlDatabase();
			defaultsPrivate->releaseDatabase(QThread::currentThread());
		}
	}

	return false;
}

// ------------- Exceptions -------------

SetupDoesNotExistException::SetupDoesNotExistException(const QString &setupName) :
	Exception(setupName, QStringLiteral("The requested setup does not exist! Create it with Setup::create"))
{}

SetupDoesNotExistException::SetupDoesNotExistException(const SetupDoesNotExistException * const other) :
	Exception(other)
{}

void SetupDoesNotExistException::raise() const
{
	throw (*this);
}

QException *SetupDoesNotExistException::clone() const
{
	return new SetupDoesNotExistException(this);
}
