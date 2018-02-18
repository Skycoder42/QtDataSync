#include "defaults.h"
#include "defaults_p.h"
#include "datastore.h"
#include "setup_p.h"
#include "exchangeengine_p.h"
#include "changeemitter_p.h"
#include "emitteradapter_p.h"

#include <QtCore/QThread>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QCoreApplication>

#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "rep_changeemitter_p_replica.h"

using namespace QtDataSync;

#define QTDATASYNC_LOG d->logger

Defaults::Defaults() :
	d(nullptr)
{}

Defaults::Defaults(const QSharedPointer<DefaultsPrivate> &d) :
	d(d)
{}

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

QUrl Defaults::remoteAddress() const
{
	return d->roAddress;
}

QRemoteObjectNode *Defaults::remoteNode() const
{
	return d->acquireNode();
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
	case Setup::ECIES_ECP_SHA3_512:
#if CRYPTOPP_VERSION >= 600
		return Setup::brainpoolP384r1;
#else
		qWarning() << "Encryption scheme" << scheme
				   << "can only be used with cryptopp > 6.0. Falling back to" << Setup::RSA_OAEP_SHA3_512;
		Q_FALLTHROUGH();
#endif
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

EmitterAdapter *Defaults::createEmitter(QObject *parent) const
{
	QObject *emitter = nullptr;
	if(d->passiveEmitter)
		emitter = d->passiveEmitter;
	else
		emitter = SetupPrivate::engine(d->setupName)->emitter();
	return new EmitterAdapter(emitter, d->cacheInfo, parent);
}

QVariant Defaults::cacheHandle() const
{
	return QVariant::fromValue(d->cacheInfo);
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

// ------------- PRIVATE IMPLEMENTATION Defaults -------------

#undef QTDATASYNC_LOG
#define QTDATASYNC_LOG logger

const QString DefaultsPrivate::DatabaseName(QStringLiteral("__QtDataSync_database_%1_0x%2"));
QMutex DefaultsPrivate::setupDefaultsMutex;
QHash<QString, QSharedPointer<DefaultsPrivate>> DefaultsPrivate::setupDefaults;
QThreadStorage<QHash<QString, quint64>> DefaultsPrivate::dbRefHash;

void DefaultsPrivate::createDefaults(const QString &setupName, bool isPassive, const QDir &storageDir, const QUrl &roAddress, const QHash<Defaults::PropertyKey, QVariant> &properties, QJsonSerializer *serializer, ConflictResolver *resolver)
{
	//create the defaults and do additional setup
	auto d = QSharedPointer<DefaultsPrivate>::create(setupName, storageDir, roAddress, properties, serializer, resolver);
	//following must be done after the constructor
	if(d->resolver)
		d->resolver->setDefaults(d);

	//final steps (must be last things done): move to the correct thread and make passive if needed
	if(d->thread() != qApp->thread())
		d->moveToThread(qApp->thread());
	if(isPassive) {
		QMetaObject::invokeMethod(d.data(), "makePassive",
								  Qt::QueuedConnection);
	}

	QMutexLocker _(&setupDefaultsMutex);
	if(setupDefaults.contains(setupName))
		throw SetupExistsException(setupName);
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
	for(auto ref : weakRefs) {
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
	auto d = setupDefaults.value(setupName);
	if(d)
		return d;
	else
		throw SetupDoesNotExistException(setupName);
}

DefaultsPrivate::DefaultsPrivate(const QString &setupName, const QDir &storageDir, const QUrl &roAddress, const QHash<Defaults::PropertyKey, QVariant> &properties, QJsonSerializer *serializer, ConflictResolver *resolver) :
	setupName(setupName),
	storageDir(storageDir),
	logger(new Logger("defaults", setupName, this)),
	roAddress(roAddress),
	serializer(serializer),
	resolver(resolver),
	properties(properties),
	roMutex(),
	roNodes(),
	cacheInfo(nullptr),
	passiveEmitter(nullptr)
{
	//parenting
	serializer->setParent(this);
	if(resolver)
		resolver->setParent(this);

	//create the default property values if unset
	if(!this->properties.contains(Defaults::SignKeyParam)) {
		this->properties.insert(Defaults::SignKeyParam,
								Defaults::defaultParam(static_cast<Setup::SignatureScheme>(this->properties.value(Defaults::SignScheme).toInt())));
	}

	if(!this->properties.contains(Defaults::CryptKeyParam)) {
		this->properties.insert(Defaults::CryptKeyParam,
								Defaults::defaultParam(static_cast<Setup::EncryptionScheme>(this->properties.value(Defaults::CryptScheme).toInt())));
	}

	//create cache
	auto maxSize = properties.value(Defaults::CacheSize).toInt();
	if(maxSize > 0)
		cacheInfo = QSharedPointer<EmitterAdapter::CacheInfo>::create(maxSize);
}

DefaultsPrivate::~DefaultsPrivate()
{
	QMutexLocker _(&roMutex);
	for(auto node : roNodes)
		node->deleteLater();
	roNodes.clear();
}

QSqlDatabase DefaultsPrivate::acquireDatabase()
{
	auto name = DefaultsPrivate::DatabaseName
				.arg(setupName)
				.arg(QString::number(reinterpret_cast<quint64>(QThread::currentThread()), 16));
	if((dbRefHash.localData()[setupName])++ == 0) {
		auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
		database.setDatabaseName(storageDir.absoluteFilePath(QStringLiteral("store.db")));
		database.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=30000;"
												  "QSQLITE_ENABLE_REGEXP"));
		if(!database.open()) {
			logFatal(QStringLiteral("Failed to open local database. Database error:\n\t") +
					 database.lastError().text());
		}

		//verify sqlite is threadsafe
		QSqlQuery pragmaThreadSafe(database);
		if(!pragmaThreadSafe.exec(QStringLiteral("PRAGMA compile_options")))
			logWarning() << "Failed to verify sqlite threadsafety";
		else {
			auto found = false;
			while(pragmaThreadSafe.next()) {
				auto tSafe = pragmaThreadSafe.value(0).toString().split(QLatin1Char('='));
				if(tSafe.size() == 2 &&
				   tSafe[0] == QStringLiteral("THREADSAFE")) {
					if(tSafe[1].toInt() == 0)
						logFatal(QStringLiteral("sqlite was NOT compiled threadsafe. This can lead to crashes and corrupted data"));
					else
						logDebug() << "Verified sqlite threadsafety";
					found = true;
					break;
				}
			}
			if(!found)
				logWarning() << "Failed to verify sqlite threadsafety";
		}

		//enable foreign keys
		QSqlQuery pragmaForeignKeys(database);
		if(!pragmaForeignKeys.exec(QStringLiteral("PRAGMA foreign_keys = ON")))
			logWarning() << "Failed to enable foreign_keys support";
	}

	return QSqlDatabase::database(name);
}

void DefaultsPrivate::releaseDatabase()
{
	if(--(dbRefHash.localData()[setupName]) == 0) {
		auto name = DefaultsPrivate::DatabaseName
					.arg(setupName)
					.arg(QString::number(reinterpret_cast<quint64>(QThread::currentThread()), 16));
		QSqlDatabase::database(name).close();
		QSqlDatabase::removeDatabase(name);
	}
}

QRemoteObjectNode *DefaultsPrivate::acquireNode()
{
	auto cThread = QThread::currentThread();
	if(!cThread)
		throw Exception(setupName, QStringLiteral("Cannot access replicated classes from a non-Qt thread"));

	QMutexLocker _(&roMutex);
	auto node = roNodes.value(cThread);
	if(!node) {
		node = new QRemoteObjectNode();
		if(!node->connectToNode(roAddress))
			throw Exception(setupName, QStringLiteral("Unable to connect to remote object host"));
		QObject::connect(cThread, &QThread::finished,
						 this, &DefaultsPrivate::roThreadDone,
						 Qt::DirectConnection); //direct connect
		roNodes.insert(cThread, node);
	}

	return node;
}

void DefaultsPrivate::roThreadDone()
{
	auto cThread = qobject_cast<QThread*>(sender());
	if(cThread) {
		QMutexLocker _(&roMutex);
		auto node = roNodes.take(cThread);
		if(node)
			node->deleteLater();
	}
}

void DefaultsPrivate::makePassive()
{
	auto node = acquireNode();
	passiveEmitter = node->acquire<ChangeEmitterReplica>();
	emit passiveCreated();
	if(passiveEmitter->isInitialized())
		emit passiveReady();
	else {
		connect(passiveEmitter, &ChangeEmitterReplica::initialized,
				this, &DefaultsPrivate::passiveReady);
	}
}

// ------------- PRIVATE IMPLEMENTATION DatabaseRef -------------

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
