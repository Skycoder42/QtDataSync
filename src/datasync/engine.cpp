#include "engine.h"
#include "engine_p.h"
#include "enginedatamodel_p.h"
#include "tabledatamodel_p.h"
using namespace QtDataSync;
namespace sph = std::placeholders;

Q_LOGGING_CATEGORY(QtDataSync::logEngine, "qt.datasync.Engine")

void Engine::syncDatabase(const QString &databaseConnection, bool autoActivateSync, bool enableLiveSync, bool addAllTables)
{
	return syncDatabase(QSqlDatabase::database(databaseConnection, true), autoActivateSync, enableLiveSync, addAllTables);
}

void Engine::syncDatabase(QSqlDatabase database, bool autoActivateSync, bool enableLiveSync, bool addAllTables)
{
	Q_D(Engine);
	if (!database.isOpen())
		throw TableException{{}, QStringLiteral("Database not open"), database.lastError()};

	auto watcher = d->getWatcher(std::move(database));
	if (autoActivateSync)
		watcher->reactivateTables(enableLiveSync);
	if (addAllTables)
		watcher->addAllTables(enableLiveSync);
}

void Engine::syncTable(const QString &table, bool enableLiveSync, const QString &databaseConnection, const QStringList &fields, const QString &primaryKeyType)
{
	return syncTable(table, enableLiveSync, QSqlDatabase::database(databaseConnection, true), fields, primaryKeyType);
}

void Engine::syncTable(const QString &table, bool enableLiveSync, QSqlDatabase database, const QStringList &fields, const QString &primaryKeyType)
{
	Q_D(Engine);
	if (!database.isOpen())
		throw TableException{{}, QStringLiteral("Database not open"), database.lastError()};
	d->getWatcher(std::move(database))->addTable(table, enableLiveSync, fields, primaryKeyType);
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
	d->getWatcher(std::move(database))->resyncAllTables(direction);
}

void Engine::resyncTable(const QString &table, ResyncMode direction, const QString &databaseConnection)
{
	resyncTable(table, direction, QSqlDatabase::database(databaseConnection, true));
}

void Engine::resyncTable(const QString &table, ResyncMode direction, QSqlDatabase database)
{
	Q_D(Engine);
	d->getWatcher(std::move(database))->resyncTable(table, direction);
}

bool Engine::isLiveSyncEnabled(const QString &table) const
{
	Q_D(const Engine);
	if (const auto tm = d->tableMachines.value(table); tm.second)
		return tm.second->isLiveSyncEnabled();
	else
		return false;
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

	d->engineModel->start();
}

void Engine::stop()
{
	Q_D(Engine);
	d->engineModel->stop();
}

void Engine::logOut()
{
	Q_D(Engine);
	d->engineModel->logOut();
}

void Engine::deleteAccount()
{
	Q_D(Engine);
	d->engineModel->deleteAccount();
}

void Engine::triggerSync(bool reconnectLiveSync)
{
	Q_D(Engine);
	for (const auto &tm : d->tableMachines)
		tm.second->triggerSync(reconnectLiveSync);
}

void Engine::setLiveSyncEnabled(bool liveSyncEnabled)
{
	Q_D(Engine);
	for (auto it = d->tableMachines.keyBegin(), end = d->tableMachines.keyEnd(); it != end; ++it)
		setLiveSyncEnabled(*it, liveSyncEnabled);
}

void Engine::setLiveSyncEnabled(const QString &table, bool liveSyncEnabled)
{
	Q_D(Engine);
	auto tm = d->tableMachines.value(table);
	if (tm.second)
		tm.second->setLiveSyncEnabled(liveSyncEnabled);
}

Engine::Engine(QScopedPointer<SetupPrivate> &&setup, QObject *parent) :
	QObject{*new EnginePrivate{}, parent}
{
	Q_D(Engine);
	d->setup.swap(setup);
	d->setup->finializeForEngine(this);

	d->connector = new RemoteConnector{this};
	d->setupStateMachine();
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
		// enforce direct connections, must be handled as part of the add/remove in the watcher
		connect(watcher, &DatabaseWatcher::tableAdded,
				this, &EnginePrivate::_q_tableAdded,
				Qt::DirectConnection);
		connect(watcher, &DatabaseWatcher::tableRemoved,
				this, &EnginePrivate::_q_tableRemoved,
				Qt::DirectConnection);
		watcherIt = watchers.insert(database.connectionName(), watcher);
	}
	return *watcherIt;
}

DatabaseWatcher *EnginePrivate::findWatcher(const QString &name)
{
	for (const auto watcher : qAsConst(watchers)) {
		if (watcher->hasTable(name))
			return watcher;
	}
	return nullptr;
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

void EnginePrivate::setupStateMachine()
{
	Q_Q(Engine);
	engineMachine = new EngineStateMachine{q};
	if (!engineMachine->init())
		throw SetupException{QStringLiteral("Failed to initialize statemachine!")};
	engineModel = static_cast<EngineDataModel*>(engineMachine->dataModel());
	Q_ASSERT(engineModel);

	connect(engineModel, &EngineDataModel::startTableSync,
			this, &EnginePrivate::_q_startTableSync);
	connect(engineModel, &EngineDataModel::stopTableSync,
			this, &EnginePrivate::_q_stopTableSync);
	connect(engineModel, &EngineDataModel::errorOccured,
			this, &EnginePrivate::_q_errorOccured);
	engineModel->setupModel(setup->authenticator, connector);

	// --- debug states ---
	if (logEngine().isDebugEnabled()) {
		QObject::connect(engineMachine, &QScxmlStateMachine::reachedStableState, q, [this]() {
			qCDebug(logEngine) << "Statemachine reached stable state:" << engineMachine->activeStateNames(false);
		});
	}

	engineMachine->start();
	qCDebug(logEngine) << "Started engine statemachine";
}

void EnginePrivate::_q_startTableSync()
{
	for (const auto &tm : qAsConst(tableMachines))
		tm.second->start();
}

void EnginePrivate::_q_stopTableSync()
{
	for (auto it = tableMachines.constBegin(), end = tableMachines.constEnd(); it != end; ++it) {
		if (it->second->isRunning())
			stopCache.insert(it.key());
		it->second->stop();
	}
}

void EnginePrivate::_q_errorOccured(const ErrorInfo &info)
{
	Q_Q(Engine);
	Q_EMIT q->errorOccured(info.type, info.message, info.data);
}

void EnginePrivate::_q_tableAdded(const QString &name, bool liveSync)
{
	Q_Q(Engine);
	const auto watcher = qobject_cast<DatabaseWatcher*>(q->sender());
	Q_ASSERT(watcher);

	auto machine = new TableStateMachine{q};
	if (!machine->init()) {
		_q_tableErrorOccured(name, {
								 ErrorType::Table,
								 Engine::tr("Failed to initialize processor"),
								 {}
							 });
		return;
	}

	auto model = static_cast<TableDataModel*>(machine->dataModel());
	Q_ASSERT(model);
	connect(model, &TableDataModel::errorOccured,
			this, &EnginePrivate::_q_tableErrorOccured);
	connect(model, &TableDataModel::stopped,
			this, &EnginePrivate::_q_tableStopped);
	QObject::connect(model, &TableDataModel::liveSyncEnabledChanged,
					 q, std::bind(&Engine::liveSyncEnabledChanged, (Engine*)q, name, sph::_1, Engine::QPrivateSignal{}));

	model->setupModel(name, watcher, connector, setup->transformer);
	machine->start();

	tableMachines.insert(name, std::make_pair(machine, model));
	model->setLiveSyncEnabled(liveSync);
	if (engineModel->isSyncActive())
		model->start();
}

void EnginePrivate::_q_tableRemoved(const QString &name)
{
	auto tm = tableMachines.take(name);
	if (tm.second->isRunning())
		tm.second->stop();
	else {
		tm.first->stop();
		tm.first->deleteLater();
	}
}

void EnginePrivate::_q_tableStopped(const QString &table)
{
	Q_Q(Engine);
	// stopped after table remove -> delete the machine
	if (!tableMachines.contains(table)) {
		auto model = qobject_cast<TableDataModel*>(q->sender());
		Q_ASSERT(model);
		auto sm = model->stateMachine();
		sm->stop();
		sm->deleteLater();
	}

	if (stopCache.remove(table) && stopCache.isEmpty())
		engineModel->allTablesStopped();
}

void EnginePrivate::_q_tableErrorOccured(const QString &table, const ErrorInfo &info)
{
	Q_Q(Engine);
	// TODO special treatment for network and live hard sync error -> reconnect on net state change?!?
	switch (info.type) {
	// global errors that need the engine to be stopped
	case Engine::ErrorType::Unknown:	// better safe then sorry
	case Engine::ErrorType::Network:	// network errors prevent syncing, until network is back
	case Engine::ErrorType::Database:	// affects the whole database -> stop syncing
		engineModel->cancel(info);
		break;
	// local errors that only affect certain datasets, tables or even nothing
	case Engine::ErrorType::Table:
		// table errors disable sync for that table, but do NOT stop the engine
		if (const auto watcher = findWatcher(table); watcher)
			watcher->dropTable(table);
		Q_FALLTHROUGH();
	case Engine::ErrorType::LiveSyncSoft:	// automatically reconnected
	case Engine::ErrorType::LiveSyncHard:	// will have automatically disabled live sync before this slot already
	case Engine::ErrorType::Entry:			// has been marked corrupted
	case Engine::ErrorType::Transform:		// has been marked corrupted
		Q_EMIT q->errorOccured(info.type, info.message, info.data);
		break;
	// special errors
	case Engine::ErrorType::Transaction:
		// transaction errors are temporary -> restart table sync (but still report the error)
		if (const auto tm = tableMachines.value(table); tm.second) {
			tm.second->stop();
			tm.second->start();
		}
		Q_EMIT q->errorOccured(info.type, info.message, info.data);
		break;
	default:
		Q_UNREACHABLE();
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
