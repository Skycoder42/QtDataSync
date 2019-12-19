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

bool Engine::syncDb(const QString &databaseConnection, const QStringList &tables)
{
	return syncDb(QSqlDatabase::database(databaseConnection, true), tables);
}

bool Engine::syncDb(QSqlDatabase database, const QStringList &tables)
{
	Q_D(Engine);
	if (!database.isOpen())
		return false;

	auto watcher = d->watcher(std::move(database));
	if (tables.isEmpty())
		return watcher->addAllTables();
	else {
		for (const auto &table : tables) {
			if (!watcher->addTable(table))
				return false;
		}
		return true;
	}
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
	return d->watcher(std::move(database))->addTable(table, fields, primaryKeyType);
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

void Engine::removeDbSync(const QString &databaseConnection, const QStringList &tables)
{
	removeDbSync(QSqlDatabase::database(databaseConnection, false), tables);
}

void Engine::removeDbSync(QSqlDatabase database, const QStringList &tables)
{
	Q_D(Engine);
	auto watcher = d->watcher(std::move(database));
	if (tables.isEmpty())
		watcher->removeAllTables();
	else {
		for (const auto &table : tables)
			watcher->removeTable(table);
	}

	if (!watcher->hasTables())
		d->removeWatcher(watcher);
}

void Engine::removeTableSync(const QString &table, const QString &databaseConnection)
{
	removeTableSync(table, QSqlDatabase::database(databaseConnection, false));
}

void Engine::removeTableSync(const QString &table, QSqlDatabase database)
{
	Q_D(Engine);
	auto watcher = d->watcher(std::move(database));
	watcher->removeTable(table);
	if (!watcher->hasTables())
		d->removeWatcher(watcher);
}

void Engine::unsyncDb(const QString &databaseConnection, const QStringList &tables)
{
	unsyncDb(QSqlDatabase::database(databaseConnection, true), tables);
}

void Engine::unsyncDb(QSqlDatabase database, const QStringList &tables)
{
	Q_D(Engine);
	auto watcher = d->watcher(std::move(database));
	if (tables.isEmpty())
		watcher->unsyncAllTables();
	else {
		for (const auto &table : tables)
			watcher->unsyncTable(table);
	}

	if (!watcher->hasTables())
		d->removeWatcher(watcher);
}

void Engine::unsyncTable(const QString &table, const QString &databaseConnection)
{
	unsyncTable(table, QSqlDatabase::database(databaseConnection, true));
}

void Engine::unsyncTable(const QString &table, QSqlDatabase database)
{
	Q_D(Engine);
	auto watcher = d->watcher(std::move(database));
	watcher->unsyncTable(table);
	if (!watcher->hasTables())
		d->removeWatcher(watcher);
}

Engine::Engine(QScopedPointer<SetupPrivate> &&setup, QObject *parent) :
	QObject{*new EnginePrivate{}, parent}
{
	Q_D(Engine);
	d->setup.swap(setup);
	d->setup->finializeForEngine(this);

	d->setupStateMachine();
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
	statemachine = new EngineStateMachine{q};
	if (!statemachine->init())
		throw SetupException{QStringLiteral("Failed to initialize statemachine!")};

	// --- states ---
	// # Active
	statemachine->connectToState(QStringLiteral("Active"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterActive, this)));
	// ## SigningIn
	connect(setup->authenticator, &IAuthenticator::signInSuccessful,
			this, &EnginePrivate::_q_signInSuccessful);
	connect(setup->authenticator, &IAuthenticator::signInFailed,
			this, &EnginePrivate::_q_handleError);
	connect(setup->authenticator, &IAuthenticator::accountDeleted,
			this, &EnginePrivate::_q_accountDeleted);
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

void EnginePrivate::setupConnector(const QString &userId)
{
	Q_Q(Engine);
	connector = new RemoteConnector{userId, q};
	connect(connector, &RemoteConnector::triggerSync,
			this, &EnginePrivate::_q_triggerSync);
	connect(connector, &RemoteConnector::syncDone,
			this, &EnginePrivate::_q_syncDone);
	connect(connector, &RemoteConnector::uploadedData,
			this, &EnginePrivate::_q_uploadedData);
	connect(connector, &RemoteConnector::networkError,
			this, &EnginePrivate::_q_handleError);
}

void EnginePrivate::fillDirtyTables()
{
	dirtyTables.clear();
	for (auto watcher : dbWatchers)
		dirtyTables.append(watcher->tables());
	if (const auto dupes = dirtyTables.removeDuplicates(); dupes > 0)
		qCWarning(logEngine) << "Detected" << dupes << "tables with identical names - this can lead to synchronization failures!";
}

DatabaseWatcher *EnginePrivate::watcher(QSqlDatabase &&database)
{
	Q_Q(Engine);
	auto &watcher = dbWatchers[database.connectionName()];
	if (!watcher)
		watcher = new DatabaseWatcher{std::move(database), q};
	return watcher;
}

void EnginePrivate::removeWatcher(DatabaseWatcher *watcher)
{
	dbWatchers.remove(watcher->database().connectionName());
	watcher->deleteLater();
}

void EnginePrivate::onEnterActive()
{
	// prepopulate all tables as dirty, so that when sync starts, all are updated
	fillDirtyTables();
}

void EnginePrivate::onEnterDownloading()
{
	if (dirtyTables.isEmpty())
		statemachine->submitEvent(QStringLiteral("dlReady"));  // done with dowloading
	else
		connector->getChanges(dirtyTables.head(), QDateTime::currentDateTimeUtc()); // TODO get lastSync from db
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
	if (!connector)
		setupConnector(userId);
	connector->setIdToken(idToken);
	statemachine->submitEvent(QStringLiteral("signedIn"));  // continue syncing, but has no effect if only token refresh
}

void EnginePrivate::_q_accountDeleted(bool success)
{
	Q_UNIMPLEMENTED();
}

void EnginePrivate::_q_triggerSync(const QString &type)
{
	if (type.isEmpty())  // empty -> all tables
		fillDirtyTables();
	else if (!dirtyTables.contains(type))  // only add given table
		dirtyTables.enqueue(type);
	statemachine->submitEvent(QStringLiteral("triggerSync"));  // does nothing if already syncing
}

void EnginePrivate::_q_syncDone(const QString &type)
{
	if (!dirtyTables.isEmpty() && dirtyTables.head() == type)
		dirtyTables.dequeue();
	if (!dirtyTables.isEmpty())
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
