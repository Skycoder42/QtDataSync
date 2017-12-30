#include "defaults.h"
#include "defaults_p.h"
#include "datastore.h"

#include <QtCore/QThread>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>
#include <QtCore/QEvent>

#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace QtDataSync;

#define QTDATASYNC_LOG d->logger

Defaults::Defaults() :
	d(nullptr)
{}

Defaults::Defaults(const QSharedPointer<DefaultsPrivate> &d) :
	d(d)
{}

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

QRemoteObjectNode *Defaults::remoteNode() const
{
	return d->node;
}

QSettings *Defaults::createSettings(QObject *parent, const QString &group) const
{
	auto path = d->storageDir.absoluteFilePath(QStringLiteral("config.ini"));
	auto settings = new QSettings(path, QSettings::IniFormat, parent);
	if(!group.isNull())
		settings->beginGroup(group);
	return settings;
}

const QJsonSerializer *Defaults::serializer() const
{
	return d->serializer;
}

const ConflictResolver *Defaults::conflictResolver() const
{
	return d->resolver;
}

QVariant Defaults::property(Defaults::PropertyKey key) const
{
	return d->properties.value(key);
}

QVariant Defaults::defaultParam(Setup::SignatureScheme scheme)
{
	switch(scheme) {
	case Setup::RSA_PSS_SHA3_512:
		return 4096;
	case Setup::ECDSA_ECP_SHA3_512:
	case Setup::ECNR_ECP_SHA3_512:
		return Setup::brainpoolP384r1;
	default:
		Q_UNREACHABLE();
		break;
	}
}

QVariant Defaults::defaultParam(Setup::EncryptionScheme scheme)
{
	switch (scheme) {
	case Setup::RSA_OAEP_SHA3_512:
		return 4096;
	default:
		Q_UNREACHABLE();
		break;
	}
}

DatabaseRef Defaults::aquireDatabase(QObject *object) const
{
	return DatabaseRef(new DatabaseRefPrivate(d, object));
}

QReadWriteLock *Defaults::databaseLock() const
{
	return &(d->lock);
}

// ------------- DatabaseRef -------------

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

DatabaseRef &DatabaseRef::operator=(DatabaseRef &&other)
{
	d.reset();
	d.swap(other.d);
	return (*this);
}

bool DatabaseRef::isValid() const
{
	return d && d->db().isValid();
}

QSqlDatabase DatabaseRef::database() const
{
	return d->db();
}

DatabaseRef::operator QSqlDatabase() const
{
	return d->db();
}

QSqlDatabase *DatabaseRef::operator->() const
{
	return &(d->db());
}

// ------------- PRIVAZE IMPLEMENTATION Defaults -------------

#undef QTDATASYNC_LOG
#define QTDATASYNC_LOG logger

const QString DefaultsPrivate::DatabaseName(QStringLiteral("__QtDataSync_database_%1_0x%2"));
QMutex DefaultsPrivate::setupDefaultsMutex;
QHash<QString, QSharedPointer<DefaultsPrivate>> DefaultsPrivate::setupDefaults;
QThreadStorage<QHash<QString, quint64>> DefaultsPrivate::dbRefHash;

void DefaultsPrivate::createDefaults(const QString &setupName, const QDir &storageDir, const QUrl &roAddress, const QHash<Defaults::PropertyKey, QVariant> &properties, QJsonSerializer *serializer, ConflictResolver *resolver)
{
	QMutexLocker _(&setupDefaultsMutex);
	auto d = QSharedPointer<DefaultsPrivate>::create(setupName, storageDir, roAddress, properties, serializer, resolver);
	if(d->resolver)
		d->resolver->setDefaults(d);

	//create the default propertie values if unset
	if(!d->properties.contains(Defaults::SignKeyParam))
		d->properties.insert(Defaults::SignKeyParam,
							 Defaults::defaultParam((Setup::SignatureScheme)d->properties.value(Defaults::SignScheme).toInt()));

	if(!d->properties.contains(Defaults::CryptKeyParam))
		d->properties.insert(Defaults::CryptKeyParam,
							 Defaults::defaultParam((Setup::EncryptionScheme)d->properties.value(Defaults::CryptScheme).toInt()));

	setupDefaults.insert(setupName, d);
}

void DefaultsPrivate::removeDefaults(const QString &setupName)
{
	QMutexLocker _(&setupDefaultsMutex);
#ifndef QT_NO_DEBUG
	QWeakPointer<DefaultsPrivate> weakRef;
	{
		auto ref = setupDefaults.take(setupName);
		if(ref)
			weakRef = ref.toWeakRef();
	}
	if(weakRef) {
#undef QTDATASYNC_LOG
#define QTDATASYNC_LOG weakRef.toStrongRef()->logger
		logCritical() << "Defaults for setup still in user after setup was deleted!";
#undef QTDATASYNC_LOG
#define QTDATASYNC_LOG logger
	}
#else
	setupDefaults.remove(setupName);
#endif
}

void DefaultsPrivate::clearDefaults()
{
	QMutexLocker _(&setupDefaultsMutex);
#ifndef QT_NO_DEBUG
	QList<QPair<QString, QWeakPointer<DefaultsPrivate>>> weakRefs;
	for(auto it = setupDefaults.constBegin(); it != setupDefaults.constEnd(); it++)
		weakRefs.append({it.key(), it.value().toWeakRef()});
	setupDefaults.clear();
	foreach(auto ref, weakRefs) {
#undef QTDATASYNC_LOG
#define QTDATASYNC_LOG ref.second.toStrongRef()->logger
		if(ref.second)
			logCritical() << "Defaults for setup still in user after setup was deleted!";
#undef QTDATASYNC_LOG
#define QTDATASYNC_LOG logger
	}
#else
	setupDefaults.clear();
#endif
}

QSharedPointer<DefaultsPrivate> DefaultsPrivate::obtainDefaults(const QString &setupName)
{
	QMutexLocker _(&setupDefaultsMutex);
	return setupDefaults.value(setupName);
}

DefaultsPrivate::DefaultsPrivate(const QString &setupName, const QDir &storageDir, const QUrl &roAddress, const QHash<Defaults::PropertyKey, QVariant> &properties, QJsonSerializer *serializer, ConflictResolver *resolver) :
	setupName(setupName),
	storageDir(storageDir),
	logger(new Logger("defaults", setupName, this)),
	node(new QRemoteObjectNode(this)),
	serializer(serializer),
	resolver(resolver),
	properties(properties),
	lock(QReadWriteLock::NonRecursive)
{
	if(!node->connectToNode(roAddress))
		logWarning() << "Failed to connect to engine remote object host node";
	serializer->setParent(this);
	if(resolver)
		resolver->setParent(this);
}

DefaultsPrivate::~DefaultsPrivate()
{
	auto cnt = dbRefHash.localData().value(setupName);
	if(cnt > 0)
		logWarning() << "Defaults destroyed with" << cnt << "open database connections!";
}

QSqlDatabase DefaultsPrivate::acquireDatabase()
{
	auto name = DefaultsPrivate::DatabaseName
				.arg(setupName)
				.arg(QString::number((quint64)QThread::currentThread(), 16));
	if((dbRefHash.localData()[setupName])++ == 0) {
		auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
		database.setDatabaseName(storageDir.absoluteFilePath(QStringLiteral("store.db")));
		if(!database.open()) {
			logFatal(QStringLiteral("Failed to open database local database. Database error:\n\t") +
					 database.lastError().text());
		}
	}

	return QSqlDatabase::database(name);
}

void DefaultsPrivate::releaseDatabase()
{
	if(--(dbRefHash.localData()[setupName]) == 0) {
		auto name = DefaultsPrivate::DatabaseName
					.arg(setupName)
					.arg(QString::number((quint64)QThread::currentThread(), 16));
		QSqlDatabase::database(name).close();
		QSqlDatabase::removeDatabase(name);
	}
}

// ------------- PRIVAZE IMPLEMENTATION DatabaseRef -------------

DatabaseRefPrivate::DatabaseRefPrivate(QSharedPointer<DefaultsPrivate> defaultsPrivate, QObject *object) :
	_defaultsPrivate(defaultsPrivate),
	_object(object),
	_database()
{
	object->installEventFilter(this);
}

DatabaseRefPrivate::~DatabaseRefPrivate()
{
	if(_database.isValid()) {
		_database = QSqlDatabase();
		_defaultsPrivate->releaseDatabase();
	}
}

QSqlDatabase &DatabaseRefPrivate::db()
{
	if(!_database.isValid())
		_database = _defaultsPrivate->acquireDatabase();
	return _database;
}

bool DatabaseRefPrivate::eventFilter(QObject *watched, QEvent *event)
{
	if(event->type() == QEvent::ThreadChange && watched == _object) {
		if(_database.isValid()) {
			_database = QSqlDatabase();
			_defaultsPrivate->releaseDatabase();
		}
	}

	return false;
}
