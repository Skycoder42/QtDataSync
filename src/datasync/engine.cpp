#include "engine.h"
#include "engine_p.h"
using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logEngine, "qt.datasync.Engine")

void Engine::syncDatabase(const QString &databaseConnection, bool autoActivateSync, bool addAllTables)
{
	return syncDatabase(QSqlDatabase::database(databaseConnection, true), autoActivateSync, addAllTables);
}

void Engine::syncDatabase(QSqlDatabase database, bool autoActivateSync, bool addAllTables)
{
	Q_D(Engine);
	if (!database.isOpen())
		throw TableException{{}, QStringLiteral("Database not open"), database.lastError()};

	auto watcher = d->getWatcher(std::move(database));
	if (autoActivateSync)
		watcher->reactivateTables();
	if (addAllTables)
		watcher->addAllTables();
}

void Engine::syncTable(const QString &table, const QString &databaseConnection, const QStringList &fields, const QString &primaryKeyType)
{
	return syncTable(table, QSqlDatabase::database(databaseConnection, true), fields, primaryKeyType);
}

void Engine::syncTable(const QString &table, QSqlDatabase database, const QStringList &fields, const QString &primaryKeyType)
{
	Q_D(Engine);
	if (!database.isOpen())
		throw TableException{{}, QStringLiteral("Database not open"), database.lastError()};
	d->getWatcher(std::move(database))->addTable(table, fields, primaryKeyType);
}

void Engine::removeDatabaseSync(const QString &databaseConnection, bool deactivateSync)
{
	removeDatabaseSync(QSqlDatabase::database(databaseConnection, false), deactivateSync);
}

void Engine::removeDatabaseSync(QSqlDatabase database, bool deactivateSync)
{
	Q_D(Engine);
	if (deactivateSync)
		d->getWatcher(std::move(database))->removeAllTables();
	else
		d->dropWatcher(database.connectionName());
}

void Engine::removeTableSync(const QString &table, const QString &databaseConnection)
{
	removeTableSync(table, QSqlDatabase::database(databaseConnection, false));
}

void Engine::removeTableSync(const QString &table, QSqlDatabase database)
{
	Q_D(Engine);
	d->getWatcher(std::move(database))->removeTable(table);
}

void Engine::unsyncDatabase(const QString &databaseConnection)
{
	unsyncDatabase(QSqlDatabase::database(databaseConnection, true));
}

void Engine::unsyncDatabase(QSqlDatabase database)
{
	Q_D(Engine);
	d->getWatcher(std::move(database))->unsyncAllTables();
}

void Engine::unsyncTable(const QString &table, const QString &databaseConnection)
{
	unsyncTable(table, QSqlDatabase::database(databaseConnection, true));
}

void Engine::unsyncTable(const QString &table, QSqlDatabase database)
{
	Q_D(Engine);
	d->getWatcher(std::move(database))->unsyncTable(table);
}

void Engine::resyncDatabase(ResyncMode direction, const QString &databaseConnection)
{
	resyncDatabase(direction, QSqlDatabase::database(databaseConnection, true));
}

void Engine::resyncDatabase(ResyncMode direction, QSqlDatabase database)
{
	Q_D(Engine);
	if (d->engineMachine->isActive(QStringLiteral("Synchronizing")))  // TODO change
		throw TableException{{}, tr("Cannot resync database while already synchronizing"), {}};
	d->getWatcher(std::move(database))->resyncAllTables(direction);
}

void Engine::resyncTable(const QString &table, ResyncMode direction, const QString &databaseConnection)
{
	resyncTable(table, direction, QSqlDatabase::database(databaseConnection, true));
}

void Engine::resyncTable(const QString &table, ResyncMode direction, QSqlDatabase database)
{
	Q_D(Engine);
	if (d->engineMachine->isActive(QStringLiteral("Synchronizing")))
		throw TableException{table, tr("Cannot resync table while already synchronizing"), {}};
	d->getWatcher(std::move(database))->resyncTable(table, direction);
}

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

	d->engineMachine->submitEvent(QStringLiteral("start"));
}

void Engine::stop()
{
	Q_D(Engine);
	d->engineMachine->submitEvent(QStringLiteral("stop"));
}

void Engine::logOut()
{
	Q_D(Engine);
	d->setup->authenticator->logOut();
	d->engineMachine->submitEvent(QStringLiteral("loggedOut")); // prompts the user to sign in again, but only if already running
}

void Engine::deleteAccount()
{
	Q_D(Engine);
	d->engineMachine->submitEvent(QStringLiteral("deleteAcc"));
}

void Engine::triggerSync()
{
	Q_D(Engine);
	for (const auto &sm : d->tableMachines)
		static_cast<TableDataModel*>(sm->dataModel())->triggerSync();
}

Engine::Engine(QScopedPointer<SetupPrivate> &&setup, QObject *parent) :
	QObject{*new EnginePrivate{}, parent}
{
	Q_D(Engine);
	d->setup.swap(setup);
	d->setup->finializeForEngine(this);

	d->connector = new RemoteConnector{this};
	d->engineMachine = new EngineStateMachine{this};
	d->setupConnections();
	d->setupStateMachine();
	d->engineMachine->start();
	qCDebug(logEngine) << "Started engine statemachine";
}



const SetupPrivate *EnginePrivate::setupFor(const Engine *engine)
{
	return engine->d_func()->setup.data();
}

DatabaseWatcher *EnginePrivate::getWatcher(QSqlDatabase &&database)
{
	Q_Q(Engine);
	auto watcherIt = watchers.find(database.connectionName());
	if (watcherIt == watchers.end()) {
		auto watcher = new DatabaseWatcher{std::move(database), q};
		connect(watcher, &DatabaseWatcher::tableAdded,
				this, &EnginePrivate::_q_tableAdded);
		connect(watcher, &DatabaseWatcher::tableRemoved,
				this, &EnginePrivate::_q_tableRemoved);
		watcherIt = watchers.insert(database.connectionName(), watcher);
	}
	return *watcherIt;
}

void EnginePrivate::dropWatcher(const QString &dbName)
{
	auto watcher = watchers.take(dbName);
	if (watcher) {
		for (const auto &table : watcher->tables())
			_q_tableRemoved(table);
		watcher->deleteLater();
	}
}

void EnginePrivate::setupConnections()
{
	// authenticator <-> engine
	connect(setup->authenticator, &IAuthenticator::signInSuccessful,
			this, &EnginePrivate::_q_signInSuccessful);
	connect(setup->authenticator, &IAuthenticator::signInFailed,
			this, &EnginePrivate::_q_handleNetError);
	connect(setup->authenticator, &IAuthenticator::accountDeleted,
			this, &EnginePrivate::_q_accountDeleted);

	// rmc <-> engine
	connect(connector, &RemoteConnector::removedUser,
			this, &EnginePrivate::_q_removedUser);
	connect(connector, &RemoteConnector::networkError,
			this, &EnginePrivate::_q_handleNetError);
}

void EnginePrivate::setupStateMachine()
{
	Q_Q(Engine);
	if (!engineMachine->init())
		throw SetupException{QStringLiteral("Failed to initialize statemachine!")};

	// --- states ---
	// # Error
	engineMachine->connectToState(QStringLiteral("Error"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterError, this)));
	// # Active
	engineMachine->connectToState(QStringLiteral("Active"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterActive, this)));
	// ## SigningIn
	engineMachine->connectToState(QStringLiteral("SigningIn"),
								 QScxmlStateMachine::onEntry(setup->authenticator, &IAuthenticator::signIn));
	// ## SignedIn
	engineMachine->connectToState(QStringLiteral("SignedIn"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterSignedIn, this)));
	engineMachine->connectToState(QStringLiteral("SignedIn"), q,
								 QScxmlStateMachine::onExit(std::bind(&EnginePrivate::onExitSignedIn, this)));
	// ### DelTables
	engineMachine->connectToState(QStringLiteral("DelTables"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterDelTables, this)));
	// ### Synchronizing
	// #### Downloading
	engineMachine->connectToState(QStringLiteral("Downloading"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterDownloading, this)));
	// ##### DlFiber
	// ###### DlRunning
	engineMachine->connectToState(QStringLiteral("DlRunning"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterDlRunning, this)));
	// ##### ProcFiber
	// ###### ProcRunning
	engineMachine->connectToState(QStringLiteral("ProcRunning"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterProcRunning, this)));
	// #### DlReady
	engineMachine->connectToState(QStringLiteral("DlReady"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterDlReady, this)));
	// #### Uploading
	engineMachine->connectToState(QStringLiteral("Uploading"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterUploading, this)));
	// ### LiveSync
	// #### LsFiber
	// ##### LsRunning
	engineMachine->connectToState(QStringLiteral("LsRunning"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterLsRunning, this)));
	// #### UlFiber
	// ##### UlRunning
	engineMachine->connectToState(QStringLiteral("UlRunning"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterUploading, this)));
	// ## DeletingAcc
	engineMachine->connectToState(QStringLiteral("DeletingAcc"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterDeletingAcc, this)));

	// --- debug states ---
	QObject::connect(engineMachine, &QScxmlStateMachine::reachedStableState, q, [this]() {
		qCDebug(logEngine) << "Statemachine reached stable state:" << engineMachine->activeStateNames(false);
	});
}

void EnginePrivate::resyncNotify(const QString &name, EnginePrivate::ResyncMode direction)
{
	if (direction.testFlag(ResyncFlag::ClearServerData)) {
		delTableQueue.enqueue(name);
		engineMachine->submitEvent(QStringLiteral("delTable"));  // only does something if synchronized
	}

	if (direction.testFlag(ResyncFlag::Download))
		dbProxy->markTableDirty(name, DatabaseProxy::Type::Cloud);

	if (direction.testFlag(ResyncFlag::Upload) ||
		direction.testFlag(ResyncFlag::CheckLocalData))
		dbProxy->markTableDirty(name, DatabaseProxy::Type::Local);
}

void EnginePrivate::activateLiveSync()
{
	if (liveSyncEnabled && engineMachine->isActive(QStringLiteral("SignedIn"))) {
		// TODO connector->startLiveSync();
		dbProxy->fillDirtyTables(DatabaseProxy::Type::Cloud);
		engineMachine->submitEvent(QStringLiteral("forceSync"));
	}
}

void EnginePrivate::onEnterError()
{
	Q_Q(Engine);
	if (lastError) {
		auto mData = *std::move(lastError);
		lastError.reset();
		Q_EMIT q->errorOccured(mData.type, mData.message, mData.data);
	} else
		engineMachine->submitEvent(QStringLiteral("stop"));  // no error -> just enter stopped instead
}

void EnginePrivate::onEnterActive()
{
	// prepopulate all tables as dirty, so that when sync starts, all are updated
	dbProxy->fillDirtyTables(DatabaseProxy::Type::Both);
}

void EnginePrivate::onEnterSignedIn()
{
	activateLiveSync();
}

void EnginePrivate::onExitSignedIn()
{
	// TODO connector->stopLiveSync();
	dlDataQueue.clear();
	lsDataQueue.clear();
	dlLastSync.clear();
	delTableQueue.clear();
}

void EnginePrivate::onEnterDelTables()
{
	if (!delTableQueue.isEmpty())
		connector->removeTable(setup->transformer->escapeType(delTableQueue.head()));
	else
		engineMachine->submitEvent(QStringLiteral("delDone"));
}

void EnginePrivate::onEnterDownloading()
{
	dlDataQueue.clear();
	dlLastSync.clear();
}

void EnginePrivate::onEnterDlRunning()
{
	if (auto dtInfo = dbProxy->nextDirtyTable(DatabaseProxy::Type::Cloud); dtInfo) {
		const auto escKey = setup->transformer->escapeType(dtInfo->first);
		QDateTime tStamp;
		if (auto dlStamp = dlLastSync.value(escKey); dlStamp.isValid()) {
			if (dtInfo->second.isValid())
				tStamp = std::max(dtInfo->second, dlStamp);
			else
				tStamp = std::move(dlStamp);
		} else
			tStamp = std::move(dtInfo->second);
		connector->getChanges(escKey, tStamp);  // has dirty table -> download it (uses latest stored or cached timestamp)
	} else
		engineMachine->submitEvent(QStringLiteral("dlReady"));  // done with dowloading
}

void EnginePrivate::onEnterProcRunning()
{
	if (!dlDataQueue.isEmpty())
		setup->transformer->transformDownload(dlDataQueue.dequeue());
	else
		engineMachine->submitEvent(QStringLiteral("procReady"));  // done with dowloading
}

void EnginePrivate::onEnterDlReady()
{
	if (liveSyncEnabled)
		engineMachine->submitEvent(QStringLiteral("triggerLiveSync"));  // enter active live sync
	else
		engineMachine->submitEvent(QStringLiteral("triggerUpload"));  // enter "normal" upload
}

void EnginePrivate::onEnterUploading()
{
	forever {
		if (const auto dtInfo = dbProxy->nextDirtyTable(DatabaseProxy::Type::Local); dtInfo) {
			if (const auto data = dbProxy->call(&DatabaseWatcher::loadData, dtInfo->first); data)
				setup->transformer->transformUpload(*data);
			else {
				dbProxy->clearDirtyTable(dtInfo->first, DatabaseProxy::Type::Local);
				continue;
			}
		} else
			engineMachine->submitEvent(QStringLiteral("syncReady"));  // no data left -> leave sync state and stay idle
		break;
	}
}

void EnginePrivate::onEnterLsRunning()
{
	if (!lsDataQueue.isEmpty())
		setup->transformer->transformDownload(lsDataQueue.dequeue());
	else
		engineMachine->submitEvent(QStringLiteral("procReady"));  // done with dowloading
}

void EnginePrivate::onEnterDeletingAcc()
{
	connector->removeUser();
}

void EnginePrivate::_q_handleNetError(const QString &errorMessage)
{
	_q_handleError(ErrorType::Network, errorMessage, {});
}

void EnginePrivate::_q_handleError(ErrorType type, const QString &errorMessage, const QVariant &errorData)
{
	if (!lastError)
		lastError = {type, errorMessage, errorData};
	qCCritical(logEngine).noquote() << type << errorMessage << errorData;
	engineMachine->submitEvent(QStringLiteral("error"));
}

void EnginePrivate::_q_handleLiveError(const QString &errorMessage, const QString &type, bool reconnect)
{
	Q_Q(Engine);
	qCCritical(logEngine).noquote() << ErrorType::LiveSyncHard << errorMessage;
	if (reconnect) {
		++lsErrorCount;
		activateLiveSync();
	} else {
		// emit error and disable live sync
		q->setLiveSyncEnabled(false);
		Q_EMIT q->errorOccured(ErrorType::LiveSyncHard, errorMessage, {});
	}
}

void EnginePrivate::_q_signInSuccessful(const QString &userId, const QString &idToken)
{
	if (!connector->isActive())
		connector->setUser(userId);
	connector->setIdToken(idToken);
	engineMachine->submitEvent(QStringLiteral("signedIn"));  // continue syncing, but has no effect if only token refresh
}

void EnginePrivate::_q_accountDeleted(bool success)
{
	if (success) {
		dbProxy->resetAll();
		engineMachine->submitEvent(QStringLiteral("stop"));
	} else {
		lastError = {ErrorType::Network, Engine::tr("Failed to delete user from authentication server"), {}};
		engineMachine->submitEvent(QStringLiteral("error"));
	}
}

void EnginePrivate::_q_triggerSync(bool uploadOnly)
{
	// TODO send other signal or not in live sync active
	if (uploadOnly)
		engineMachine->submitEvent(QStringLiteral("triggerUpload"));  // does nothing if already syncing
	else
		engineMachine->submitEvent(QStringLiteral("triggerSync"));  // does nothing if already syncing
}

void EnginePrivate::_q_syncDone(const QString &type)
{
	dbProxy->clearDirtyTable(setup->transformer->unescapeType(type), DatabaseProxy::Type::Cloud);
	engineMachine->submitEvent(QStringLiteral("dlContinue"));  // enters dl state again and downloads next table
}

void EnginePrivate::_q_downloadedData(const QString &type, const QList<CloudData> &data)
{
	if (true) {  // TODO
		if (engineMachine->isActive(QStringLiteral("SignedIn"))) {
			// store any data from livesync, as long as signed in
			lsDataQueue.append(data);
			if (engineMachine->isActive(QStringLiteral("LiveSync")))  // only process data in LiveSync state
				engineMachine->submitEvent(QStringLiteral("dataReady"));
		} else
			qCDebug(logEngine) << "Ignoring livesync data in invalid state";
	} else {
		if (engineMachine->isActive(QStringLiteral("Downloading"))) {
			if (!data.isEmpty()) {
				dlDataQueue.append(data);
				const auto ldata = data.last();
				dlLastSync.insert(ldata.key().typeName, ldata.uploaded());
			}
			engineMachine->submitEvent(QStringLiteral("dataReady"));  // starts data processing if not already running
		} else
			qCDebug(logEngine) << "Ignoring downloaded data in invalid state";
	}
}

void EnginePrivate::_q_uploadedData(const ObjectKey &key, const QDateTime &modified)
{
	dbProxy->call(&DatabaseWatcher::markUnchanged, setup->transformer->unescapeKey(key), modified);
	engineMachine->submitEvent(QStringLiteral("ulContinue"));  // always send ulContinue, the onEntry will decide if there is data end exit if not
}

void EnginePrivate::_q_triggerCloudSync(const QString &type)
{
	// happens only while uploading -> ignore in live sync (as live sync gets changes anyway)
	if (!engineMachine->isActive(QStringLiteral("LiveSync")))
		dbProxy->markTableDirty(setup->transformer->unescapeType(type), DatabaseProxy::Type::Cloud);
}

void EnginePrivate::_q_removedTable(const QString &type)
{
	if (!delTableQueue.isEmpty() && delTableQueue.head() == setup->transformer->unescapeType(type))
		delTableQueue.dequeue();
	engineMachine->submitEvent(QStringLiteral("delContinue"));
}

void EnginePrivate::_q_removedUser()
{
	setup->authenticator->deleteUser();
}

void EnginePrivate::_q_tableAdded(const QString &name)
{

}

void EnginePrivate::_q_tableRemoved(const QString &name)
{

}

void EnginePrivate::_q_transformDownloadDone(const LocalData &data)
{
	if (engineMachine->isActive(QStringLiteral("Downloading")) ||
		engineMachine->isActive(QStringLiteral("LiveSync"))) {
		dbProxy->call(&DatabaseWatcher::storeData, data);
		engineMachine->submitEvent(QStringLiteral("procContinue"));  // starts processing of the next downloaded data
	}
}



TableException::TableException(QString table, QString message, QSqlError error) :
	_table{std::move(table)},
	_message{std::move(message)},
	_error{std::move(error)}
{}

QString TableException::qWhat() const
{
	return _table.isEmpty() ?
		QStringLiteral("Error on database: %1").arg(_message) :
		QStringLiteral("Error on table %1: %2").arg(_table, _message);
}

QString TableException::message() const
{
	return _message;
}

QString TableException::table() const
{
	return _table;
}

QSqlError TableException::sqlError() const
{
	return _error;
}

void TableException::raise() const
{
	throw *this;
}

ExceptionBase *TableException::clone() const
{
	return new TableException{*this};
}

#include "moc_engine.cpp"
