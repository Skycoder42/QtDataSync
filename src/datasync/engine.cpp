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

void Engine::syncDatabase(const QString &databaseConnection, bool autoActivateSync, bool addAllTables)
{
	return syncDatabase(QSqlDatabase::database(databaseConnection, true), autoActivateSync, addAllTables);
}

void Engine::syncDatabase(QSqlDatabase database, bool autoActivateSync, bool addAllTables)
{
	Q_D(Engine);
	if (!database.isOpen())
		throw TableException{{}, QStringLiteral("Database not open"), database.lastError()};

	auto watcher = d->dbProxy->watcher(std::move(database));
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
	d->dbProxy->watcher(std::move(database))->addTable(table, fields, primaryKeyType);
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

void Engine::resyncDatabase(ResyncMode direction, const QString &databaseConnection)
{
	resyncDatabase(direction, QSqlDatabase::database(databaseConnection, true));
}

void Engine::resyncDatabase(ResyncMode direction, QSqlDatabase database)
{
	Q_D(Engine);
	if (d->statemachine->isActive(QStringLiteral("Synchronizing")))
		throw TableException{{}, tr("Cannot resync database while already synchronizing"), {}};
	const auto tables = d->dbProxy->watcher(std::move(database))->resyncAllTables(direction);
	for (const auto &table : tables)
		d->resyncNotify(table, direction);
}

void Engine::resyncTable(const QString &table, ResyncMode direction, const QString &databaseConnection)
{
	resyncTable(table, direction, QSqlDatabase::database(databaseConnection, true));
}

void Engine::resyncTable(const QString &table, ResyncMode direction, QSqlDatabase database)
{
	Q_D(Engine);
	if (d->statemachine->isActive(QStringLiteral("Synchronizing")))
		throw TableException{table, tr("Cannot resync table while already synchronizing"), {}};
	d->dbProxy->watcher(std::move(database))->resyncTable(table, direction);
	d->resyncNotify(table, direction);
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

void Engine::logOut()
{
	Q_D(Engine);
	d->setup->authenticator->logOut();
	d->statemachine->submitEvent(QStringLiteral("loggedOut")); // prompts the user to sign in again, but only if already running
}

void Engine::deleteAccount()
{
	Q_D(Engine);
	d->statemachine->submitEvent(QStringLiteral("deleteAcc"));
}

void Engine::triggerSync()
{
	Q_D(Engine);
	d->statemachine->submitEvent(QStringLiteral("triggerSync"));
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
	d->setupConnections();
	d->setupStateMachine();
	d->statemachine->start();
	qCDebug(logEngine) << "Started engine statemachine";
}



const SetupPrivate *EnginePrivate::setupFor(const Engine *engine)
{
	return engine->d_func()->setup.data();
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

	// dbProx <-> engine
	connect(dbProxy, &DatabaseProxy::triggerSync,
			this, &EnginePrivate::_q_triggerSync);
	connect(dbProxy, &DatabaseProxy::databaseError,
			this, &EnginePrivate::_q_handleError);

	// rmc <-> engine
	connect(connector, &RemoteConnector::downloadedData,
			this, &EnginePrivate::_q_downloadedData);
	connect(connector, &RemoteConnector::syncDone,
			this, &EnginePrivate::_q_syncDone);
	connect(connector, &RemoteConnector::uploadedData,
			this, &EnginePrivate::_q_uploadedData);
	connect(connector, &RemoteConnector::removedTable,
			this, &EnginePrivate::_q_removedTable);
	connect(connector, &RemoteConnector::removedUser,
			this, &EnginePrivate::_q_removedUser);
	connect(connector, &RemoteConnector::networkError,
			this, &EnginePrivate::_q_handleNetError);

	// transformer <-> engine
	connect(setup->transformer, &ICloudTransformer::transformDownloadDone,
			this, &EnginePrivate::_q_transformDownloadDone);

	// rmc <-> dbProxy
	QObject::connect(connector, &RemoteConnector::triggerSync,
					 dbProxy, [this](const QString &tableName) {
						 dbProxy->markTableDirty(tableName, DatabaseProxy::Type::Cloud);
					 });

	// rmc <-> transformer
	QObject::connect(setup->transformer, &ICloudTransformer::transformUploadDone,
					 connector, &RemoteConnector::uploadChange);
}

void EnginePrivate::setupStateMachine()
{
	Q_Q(Engine);
	if (!statemachine->init())
		throw SetupException{QStringLiteral("Failed to initialize statemachine!")};

	// --- states ---
	// # Error
	statemachine->connectToState(QStringLiteral("Error"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterError, this)));
	// # Active
	statemachine->connectToState(QStringLiteral("Active"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterActive, this)));
	// ## SigningIn
	statemachine->connectToState(QStringLiteral("SigningIn"),
								 QScxmlStateMachine::onEntry(setup->authenticator, &IAuthenticator::signIn));
	// ## SignedIn
	statemachine->connectToState(QStringLiteral("SignedIn"), q,
								 QScxmlStateMachine::onExit(std::bind(&EnginePrivate::onExitSignedIn, this)));
	// ### DelTables
	statemachine->connectToState(QStringLiteral("DelTables"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterDelTables, this)));
	// ### Synchronizing
	// #### Downloading
	statemachine->connectToState(QStringLiteral("Downloading"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterDownloading, this)));
	// ##### DlFiber
	// ###### DlRunning
	statemachine->connectToState(QStringLiteral("DlRunning"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterDlRunning, this)));
	// ##### ProcFiber
	// ###### ProcRunning
	statemachine->connectToState(QStringLiteral("ProcRunning"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterProcRunning, this)));
	// #### Uploading
	statemachine->connectToState(QStringLiteral("Uploading"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterUploading, this)));
	// ## DeletingAcc
	statemachine->connectToState(QStringLiteral("DeletingAcc"), q,
								 QScxmlStateMachine::onEntry(std::bind(&EnginePrivate::onEnterDeletingAcc, this)));

	// --- debug states ---
	QObject::connect(statemachine, &QScxmlStateMachine::reachedStableState, q, [this]() {
		qCDebug(logEngine) << "Statemachine reached stable state:" << statemachine->activeStateNames(false);
	});
}

void EnginePrivate::resyncNotify(const QString &name, EnginePrivate::ResyncMode direction)
{
	if (direction.testFlag(ResyncFlag::ClearServerData)) {
		delTableQueue.enqueue(name);
		statemachine->submitEvent(QStringLiteral("delTable"));  // only does something if synchronized
	}

	if (direction.testFlag(ResyncFlag::Download))
		dbProxy->markTableDirty(name, DatabaseProxy::Type::Cloud);

	if (direction.testFlag(ResyncFlag::Upload) ||
		direction.testFlag(ResyncFlag::CheckLocalData))
		dbProxy->markTableDirty(name, DatabaseProxy::Type::Local);
}

void EnginePrivate::onEnterError()
{
	Q_Q(Engine);
	if (lastError) {
		auto mData = *std::move(lastError);
		lastError.reset();
		Q_EMIT q->errorOccured(mData.type, mData.message, mData.data);
	} else
		statemachine->submitEvent(QStringLiteral("stop"));  // no error -> just enter stopped instead
}

void EnginePrivate::onEnterActive()
{
	// prepopulate all tables as dirty, so that when sync starts, all are updated
	dbProxy->fillDirtyTables(DatabaseProxy::Type::Both);
}

void EnginePrivate::onExitSignedIn()
{
	delTableQueue.clear();
}

void EnginePrivate::onEnterDelTables()
{
	if (!delTableQueue.isEmpty())
		connector->removeTable(delTableQueue.head());
	else
		statemachine->submitEvent(QStringLiteral("delDone"));
}

void EnginePrivate::onEnterDownloading()
{
	dlDataQueue.clear();
	dlLastSync.clear();
}

void EnginePrivate::onEnterDlRunning()
{
	if (auto dtInfo = dbProxy->nextDirtyTable(DatabaseProxy::Type::Cloud); dtInfo) {
		QDateTime tStamp;
		if (auto dlStamp = dlLastSync.value(dtInfo->first); dlStamp.isValid()) {
			if (dtInfo->second.isValid())
				tStamp = std::max(dtInfo->second, dlStamp);
			else
				tStamp = std::move(dlStamp);
		} else
			tStamp = std::move(dtInfo->second);
		connector->getChanges(dtInfo->first, tStamp);  // has dirty table -> download it (uses latest stored or cached timestamp)
	} else
		statemachine->submitEvent(QStringLiteral("dlReady"));  // done with dowloading
}

void EnginePrivate::onEnterProcRunning()
{
	if (!dlDataQueue.isEmpty())
		setup->transformer->transformDownload(dlDataQueue.dequeue());
	else
		statemachine->submitEvent(QStringLiteral("procReady"));  // done with dowloading
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
			statemachine->submitEvent(QStringLiteral("syncReady"));  // no data left -> leave sync state and stay idle
		break;
	}
}

void EnginePrivate::onEnterDeletingAcc()
{
	connector->removeUser();
}

void EnginePrivate::_q_handleError(ErrorType type, const QString &errorMessage, const QVariant &errorData)
{
	if (!lastError)
		lastError = {type, errorMessage, errorData};
	qCCritical(logEngine).noquote() << type << errorMessage << errorData;
	statemachine->submitEvent(QStringLiteral("error"));
}

void EnginePrivate::_q_handleNetError(const QString &errorMessage)
{
	_q_handleError(ErrorType::Network, errorMessage, {});
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
	if (success) {
		dbProxy->resetAll();
		statemachine->submitEvent(QStringLiteral("stop"));
	} else {
		lastError = {ErrorType::Network, Engine::tr("Failed to delete user from authentication server"), {}};
		statemachine->submitEvent(QStringLiteral("error"));
	}
}

void EnginePrivate::_q_triggerSync()
{
	statemachine->submitEvent(QStringLiteral("triggerSync"));  // does nothing if already syncing
}

void EnginePrivate::_q_syncDone(const QString &type)
{
	dbProxy->clearDirtyTable(type, DatabaseProxy::Type::Cloud);
	statemachine->submitEvent(QStringLiteral("dlContinue"));  // enters dl state again and downloads next table
}

void EnginePrivate::_q_downloadedData(const QList<CloudData> &data, bool liveSyncData)
{
	if (statemachine->isActive(QStringLiteral("Downloading"))) {
		if (!data.isEmpty()) {
			dlDataQueue.append(data);
			const auto ldata = data.last();
			dlLastSync.insert(ldata.key().typeName, ldata.uploaded());
		}
		statemachine->submitEvent(QStringLiteral("dataReady"));  // starts data processing if not already running
	} else if (liveSyncData) {
		// TODO ???
		for (const auto &d : data)
			qCDebug(logEngine) << "LIVE-SYNC-DATA" << d.key();
	} else
		qCDebug(logEngine) << "ignoring downloaded data in invalid state";
}

void EnginePrivate::_q_uploadedData(const ObjectKey &key, const QDateTime &modified)
{
	dbProxy->call(&DatabaseWatcher::markUnchanged, key, modified);
	statemachine->submitEvent(QStringLiteral("ulContinue"));  // always send ulContinue, the onEntry will decide if there is data end exit if not
}

void EnginePrivate::_q_removedTable(const QString &name)
{
	if (!delTableQueue.isEmpty() && delTableQueue.head() == name)
		delTableQueue.dequeue();
	statemachine->submitEvent(QStringLiteral("delContinue"));
}

void EnginePrivate::_q_removedUser()
{
	setup->authenticator->deleteUser();
}

void EnginePrivate::_q_transformDownloadDone(const LocalData &data)
{
	if (statemachine->isActive(QStringLiteral("Downloading"))) {
		dbProxy->call(&DatabaseWatcher::storeData, data);
		statemachine->submitEvent(QStringLiteral("procContinue"));  // starts processing of the next downloaded data
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
