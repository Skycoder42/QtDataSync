#include "engine.h"
#include "engine_p.h"
using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logEngine, "qt.datasync.Engine")

IAuthenticator *Engine::authenticator() const
{
	Q_D(const Engine);
	return d->setup->authenticator;
}

ICloudTransformer *Engine::transformer() const
{
	Q_D(const Engine);
	return d->setup->transformer;
}

bool Engine::syncDatabase(const QString &databaseConnection, bool autoActivateSync, bool addAllTables)
{
	return syncDatabase(QSqlDatabase::database(databaseConnection, true), autoActivateSync, addAllTables);
}

bool Engine::syncDatabase(QSqlDatabase database, bool autoActivateSync, bool addAllTables)
{
	Q_D(Engine);
	if (!database.isOpen())
		return false;

	auto watcher = d->dbProxy->watcher(std::move(database));
	if (autoActivateSync && !watcher->reactivateTables())
		return false;
	if (addAllTables && !watcher->addAllTables())
		return false;
	return true;
}

bool Engine::syncTable(const QString &table, const QString &databaseConnection, const QStringList &fields, const QString &primaryKeyType)
{
	return syncTable(table, QSqlDatabase::database(databaseConnection, true), fields, primaryKeyType);
}

bool Engine::syncTable(const QString &table, QSqlDatabase database, const QStringList &fields, const QString &primaryKeyType)
{
	Q_D(Engine);
	if (!database.isOpen())
		return false;
	return d->dbProxy->watcher(std::move(database))->addTable(table, fields, primaryKeyType);
}

void Engine::start()
{
	Q_D(Engine);

#ifndef QTDATASYNC_NO_NTP
	// start NTP sync if enabled
	// TODO link to engine
	if (!d->setup->ntpAddress.isEmpty()) {
		d->ntpSync = new NtpSync{this};
		d->ntpSync->syncWith(d->setup->ntpAddress, d->setup->ntpPort);
	}
#endif

	d->statemachine->submitEvent(QStringLiteral("start"));
}

void Engine::stop()
{
	Q_D(Engine);
	d->statemachine->submitEvent(QStringLiteral("stop"));
}

void Engine::removeDatabaseSync(const QString &databaseConnection, bool deactivateSync)
{
	removeDatabaseSync(QSqlDatabase::database(databaseConnection, false), deactivateSync);
}

void Engine::removeDatabaseSync(QSqlDatabase database, bool deactivateSync)
{
	Q_D(Engine);
	if (deactivateSync)
		d->dbProxy->watcher(std::move(database))->removeAllTables();
	else
		d->dbProxy->dropWatcher(std::move(database));
}

void Engine::removeTableSync(const QString &table, const QString &databaseConnection)
{
	removeTableSync(table, QSqlDatabase::database(databaseConnection, false));
}

void Engine::removeTableSync(const QString &table, QSqlDatabase database)
{
	Q_D(Engine);
	d->dbProxy->watcher(std::move(database))->removeTable(table);
}

void Engine::unsyncDatabase(const QString &databaseConnection)
{
	unsyncDatabase(QSqlDatabase::database(databaseConnection, true));
}

void Engine::unsyncDatabase(QSqlDatabase database)
{
	Q_D(Engine);
	d->dbProxy->watcher(std::move(database))->unsyncAllTables();
}

void Engine::unsyncTable(const QString &table, const QString &databaseConnection)
{
	unsyncTable(table, QSqlDatabase::database(databaseConnection, true));
}

void Engine::unsyncTable(const QString &table, QSqlDatabase database)
{
	Q_D(Engine);
	d->dbProxy->watcher(std::move(database))->unsyncTable(table);
}

Engine::Engine(QScopedPointer<SetupPrivate> &&setup, QObject *parent) :
	QObject{*new EnginePrivate{}, parent}
{
	Q_D(Engine);
	d->setup.swap(setup);
	d->setup->finializeForEngine(this);

	d->statemachine = new EngineStateMachine{this};
	d->dbProxy = new DatabaseProxy{this};
	d->connector = new RemoteConnector{this};
	d->setupStateMachine();
	d->setupConnections();
	d->statemachine->start();
	qCDebug(logEngine) << "Started engine statemachine";
}



const SetupPrivate *EnginePrivate::setupFor(const Engine *engine)
{
	return engine->d_func()->setup.data();
}

void EnginePrivate::setupStateMachine()
{
	Q_Q(Engine);
	if (!statemachine->init())
		throw SetupException{QStringLiteral("Failed to initialize statemachine!")};

	// --- states ---
	// # Active
	statemachine->connectToState(QStringLiteral("Active"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterActive, this)));
	// ## SigningIn
	statemachine->connectToState(QStringLiteral("SigningIn"),
								 QScxmlStateMachine::onEntry(setup->authenticator, &IAuthenticator::signIn));
	// ## Synchronizing
	// ### Downloading
	statemachine->connectToState(QStringLiteral("Downloading"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterDownloading, this)));
	// ### Uploading
	statemachine->connectToState(QStringLiteral("Uploading"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterUploading, this)));

	// --- debug states ---
	QObject::connect(statemachine, &QScxmlStateMachine::reachedStableState, q, [this]() {
		qCDebug(logEngine) << "Statemachine reached stable state:" << statemachine->activeStateNames(true);
	});
}

void EnginePrivate::setupConnections()
{
	// authenticator <-> engine
	connect(setup->authenticator, &IAuthenticator::signInSuccessful,
			this, &EnginePrivate::_q_signInSuccessful);
	connect(setup->authenticator, &IAuthenticator::signInFailed,
			this, &EnginePrivate::_q_handleError);
	connect(setup->authenticator, &IAuthenticator::accountDeleted,
			this, &EnginePrivate::_q_accountDeleted);

	// dbProx <-> engine
	connect(dbProxy, &DatabaseProxy::triggerSync,
			this, &EnginePrivate::_q_triggerSync);
	connect(dbProxy, &DatabaseProxy::databaseError,
			this, &EnginePrivate::_q_handleError);

	// rmc -> engine
	connect(connector, &RemoteConnector::syncDone,
			this, &EnginePrivate::_q_syncDone);
	connect(connector, &RemoteConnector::uploadedData,
			this, &EnginePrivate::_q_uploadedData);
	connect(connector, &RemoteConnector::networkError,
			this, &EnginePrivate::_q_handleError);

	// rmc <-> dbProxy
	QObject::connect(connector, &RemoteConnector::triggerSync,
					 dbProxy, &DatabaseProxy::markTableDirty);

	// rmc <-> transformer
	QObject::connect(connector, &RemoteConnector::downloadedData,
					 setup->transformer, &ICloudTransformer::transformDownload);
	QObject::connect(setup->transformer, &ICloudTransformer::transformUploadDone,
					 connector, &RemoteConnector::uploadChange);

	// transformer <-> dbProxy
}

void EnginePrivate::onEnterActive()
{
	// prepopulate all tables as dirty, so that when sync starts, all are updated
	dbProxy->fillDirtyTables();
}

void EnginePrivate::onEnterDownloading()
{
	if (dbProxy->hasDirtyTables())
		connector->getChanges(dbProxy->nextDirtyTable(), QDateTime::currentDateTimeUtc()); // TODO get lastSync from db
	else
		statemachine->submitEvent(QStringLiteral("dlReady"));  // done with dowloading
}

void EnginePrivate::onEnterUploading()
{
	// TODO get next change and upload it
	statemachine->submitEvent(QStringLiteral("syncReady"));  // no data left -> leave sync state and stay idle
}

void EnginePrivate::_q_handleError(const QString &errorMessage)
{
	lastError = errorMessage;
	qCCritical(logEngine).noquote() << errorMessage;
	statemachine->submitEvent(QStringLiteral("error"));
}

void EnginePrivate::_q_signInSuccessful(const QString &userId, const QString &idToken)
{
	if (!connector->isActive())
		connector->setUser(userId);
	connector->setIdToken(idToken);
	statemachine->submitEvent(QStringLiteral("signedIn"));  // continue syncing, but has no effect if only token refresh
}

void EnginePrivate::_q_accountDeleted(bool success)
{
	Q_UNIMPLEMENTED();
}

void EnginePrivate::_q_triggerSync()
{
	statemachine->submitEvent(QStringLiteral("triggerSync"));  // does nothing if already syncing
}

void EnginePrivate::_q_syncDone(const QString &type)
{
	dbProxy->clearDirtyTable(type);
	if (dbProxy->hasDirtyTables())
		statemachine->submitEvent(QStringLiteral("dlContinue"));  // enters dl state again and downloads next table
	else
		statemachine->submitEvent(QStringLiteral("dlReady"));  // enters ul state and starts uploading
}

void EnginePrivate::_q_uploadedData(const ObjectKey &key)
{
	// TODO mark data uploaded and continue
	statemachine->submitEvent(QStringLiteral("ulContinue"));  // always send ulContinue, the onEntry will decide if there is data end exit if not
}

#include "moc_engine.cpp"
