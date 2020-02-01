#include "engine.h"
#include "engine_p.h"
#include "setup.h"
#include "tablesynccontroller.h"
#include "enginedatamodel_p.h"
#include "tabledatamodel_p.h"
#include <QtCore/QEventLoop>
using namespace QtDataSync;
using namespace QtDataSync::__private;
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

void Engine::syncTable(const QString &table, bool enableLiveSync, const QString &databaseConnection)
{
	return syncTable(table, enableLiveSync, QSqlDatabase::database(databaseConnection, true));
}

void Engine::syncTable(const QString &table, bool enableLiveSync, QSqlDatabase database)
{
	TableConfig config{table, database};
	config.setLiveSyncEnabled(enableLiveSync);
	syncTable(config);
}

void Engine::syncTable(const TableConfig &config)
{
	Q_D(Engine);
	if (!config.connection().isOpen())
		throw TableException{config.table(), QStringLiteral("Database not open"), config.connection().lastError()};
	if (d->tableMachines.contains(config.table()))
		throw TableException{config.table(), QStringLiteral("Table already synchronized"), {}};
	d->getWatcher(config.connection())->addTable(config);
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

QSettings *Engine::syncSettings(QObject *parent)
{
	return syncSettings(true, parent);
}

QSettings *Engine::syncSettings(bool enableLiveSync, QObject *parent)
{
	return syncSettings(enableLiveSync, QSqlDatabase::database(), parent);
}

QSettings *Engine::syncSettings(bool enableLiveSync, const QString &databaseConnection, QObject *parent)
{
	return syncSettings(enableLiveSync, QSqlDatabase::database(databaseConnection, true), parent);
}

QSettings *Engine::syncSettings(bool enableLiveSync, QSqlDatabase database, QObject *parent)
{
	return syncSettings(enableLiveSync, database, QStringLiteral("qtdatasync_settings"), parent);
}

QSettings *Engine::syncSettings(bool enableLiveSync, const QString &databaseConnection, const QString &tableName, QObject *parent)
{
	return syncSettings(enableLiveSync, QSqlDatabase::database(databaseConnection, true), tableName, parent);
}

QSettings *Engine::syncSettings(bool enableLiveSync, QSqlDatabase database, QString tableName, QObject *parent)
{
	Q_D(Engine);
	// register/get the format
	const auto format = SettingsAdaptor::registerFormat();
	if (format == QSettings::InvalidFormat) {
		qCCritical(logEngine) << "Failed to register settings format for the datasync SQL settings tables";
		return nullptr;
	}

	// get the adaptor
	auto adaptor = d->settingsAdaptors.value(tableName);
	if (!adaptor) {
		// create settings table
		if (!SettingsAdaptor::createTable(database, tableName))
			return nullptr;
		// synchronize settings table
		try {
			TableConfig config{tableName, database};
			config.setLiveSyncEnabled(enableLiveSync);
			config.setVersion(QVersionNumber::fromString(QStringLiteral(QT_DATASYNC_SETTINGS_VERSION)));
			syncTable(config);
		} catch (TableException &e) {
			qCCritical(logEngine) << e.what();
			return nullptr;
		}
		// create and store adaptor
		adaptor = new SettingsAdaptor{this, std::move(tableName), database};
		d->settingsAdaptors.insert(tableName, adaptor);
	}

	// create settings
	return new QSettings{adaptor->path(), format, parent};
}

QSqlDatabase Engine::database(const QString &table) const
{
	Q_D(const Engine);
	const auto watcher = d->findWatcher(table);
	return watcher ? watcher->database() : QSqlDatabase{};
}

TableSyncController *Engine::createController(QString table, QObject *parent) const
{
	Q_D(const Engine);
	if (const auto tInfo = d->tableMachines.value(table); tInfo.model)
		return new TableSyncController{std::move(table), tInfo.model, parent};
	else
		return nullptr;
}

IAuthenticator *Engine::authenticator() const
{
	Q_D(const Engine);
	return d->authenticator->authenticator();
}

ICloudTransformer *Engine::transformer() const
{
	Q_D(const Engine);
	return d->transformer;
}

Engine::EngineState Engine::state() const
{
	Q_D(const Engine);
	return d->engineModel->state();
}

bool Engine::isRunning() const
{
	Q_D(const Engine);
	switch (d->engineModel->state()) {
	case EngineState::Inactive:
	case EngineState::Error:
	case EngineState::Invalid:
		return false;
	case EngineState::SigningIn:
	case EngineState::TableSync:
	case EngineState::Stopping:
	case EngineState::DeletingAcc:
		return true;
	default:
		Q_UNREACHABLE();
	}
}

bool Engine::waitForStopped(std::optional<std::chrono::milliseconds> timeout) const
{
	if (!isRunning())
		return true;

	QEventLoop stopLoop;
	connect(this, &Engine::stateChanged,
			&stopLoop, [&](Engine::EngineState state){
				if (state == EngineState::Inactive ||
					state == EngineState::Error)
					stopLoop.quit();
			});

	if (timeout) {
		auto timer = new QTimer{&stopLoop};
		timer->setSingleShot(true);
		connect(timer, &QTimer::timeout,
				&stopLoop, [&]() {
					stopLoop.exit(EXIT_FAILURE);
				});
		timer->start(*timeout);
	}

	return stopLoop.exec() == EXIT_SUCCESS;
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
		tm.model->triggerSync(reconnectLiveSync);
}

void Engine::setLiveSyncEnabled(bool liveSyncEnabled)
{
	Q_D(Engine);
	for (auto it = d->tableMachines.begin(), end = d->tableMachines.end(); it != end; ++it)
		it->model->setLiveSyncEnabled(liveSyncEnabled);
}

Engine::Engine(QScopedPointer<SetupPrivate> &&setup, QObject *parent) :
	QObject{*new EnginePrivate{}, parent}
{
	Q_D(Engine);
	d->setup.swap(setup);
	d->setup->finializeForEngine(this);
	d->authenticator = new FirebaseAuthenticator {
		d->setup->createAuthenticator(this),
		d->setup->firebase.apiKey,
		d->setup->settings,
		d->setup->nam,
		d->setup->sslConfig,
		this
	};
	d->transformer = d->setup->createTransformer(this);
	d->connector = new RemoteConnector {
		d->setup->firebase,
		d->setup->settings,
		d->setup->nam,
		d->setup->sslConfig,
		this
	};

	// create async watcher and enable remoting
	d->asyncWatcher = new AsyncWatcherBackend{this};
	if (d->setup->roNode) {
		d->setup->roNode->enableRemoting(d->asyncWatcher);
		qCDebug(logSetup) << "Enabled remoting of async watcher API";
	}

	d->setupStateMachine();
}



AsyncWatcherBackend *EnginePrivate::obtainAWB(Engine *engine)
{
	return engine->d_func()->asyncWatcher;
}

DatabaseWatcher *EnginePrivate::getWatcher(QSqlDatabase &&database)
{
	Q_Q(Engine);
	auto watcherIt = watchers.find(database.connectionName());
	if (watcherIt == watchers.end()) {
		auto watcher = new DatabaseWatcher{std::move(database), setup->database, q};
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

DatabaseWatcher *EnginePrivate::findWatcher(const QString &name) const
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
	engineModel = new EngineDataModel{engineMachine};
	engineMachine->setDataModel(engineModel);
	if (!engineMachine->init())
		throw SetupException{QStringLiteral("Failed to initialize statemachine!")};

	connect(engineModel, &EngineDataModel::startTableSync,
			this, &EnginePrivate::_q_startTableSync);
	connect(engineModel, &EngineDataModel::stopTableSync,
			this, &EnginePrivate::_q_stopTableSync);
	connect(engineModel, &EngineDataModel::errorOccured,
			this, &EnginePrivate::_q_errorOccured);
	QObject::connect(engineModel, &EngineDataModel::stateChanged,
					 q, std::bind(&Engine::stateChanged, q, sph::_1, Engine::QPrivateSignal{}));
	engineModel->setupModel(authenticator, connector);

	engineMachine->start();
	qCDebug(logEngine) << "Started engine statemachine";
}

void EnginePrivate::_q_startTableSync()
{
	for (const auto &tm : qAsConst(tableMachines)) {
		tm.machine->start();
		tm.model->start(true);
	}
}

void EnginePrivate::_q_stopTableSync()
{
	for (auto it = tableMachines.constBegin(), end = tableMachines.constEnd(); it != end; ++it) {
		it->model->exit();
		if (it->machine->isRunning())
			stopCache.insert(it.key());
	}
	if (stopCache.isEmpty())
		engineModel->allTablesStopped();
}

void EnginePrivate::_q_errorOccured(const ErrorInfo &info)
{
	Q_Q(Engine);
	Q_EMIT q->errorOccured(info.type, info.data);
}

void EnginePrivate::_q_tableAdded(const QString &name, bool liveSync)
{
	Q_Q(Engine);
	const auto watcher = qobject_cast<DatabaseWatcher*>(q->sender());
	Q_ASSERT(watcher);

	if (tableMachines.contains(name)) {
		qCWarning(logEngine) << "Table with name" << name
							 << "already synchronized. Cannot be added again!";
		watcher->dropTable(name);
		return;
	}

	auto machine = new TableStateMachine{q};
	auto model = new TableDataModel{machine};
	machine->setDataModel(model);
	if (!machine->init()) {
		_q_tableErrorOccured(name, {
								 ErrorType::Table,
								 {}
							 });
		return;
	}

	Q_ASSERT(model);
	connect(model, &TableDataModel::exited,
			this, &EnginePrivate::_q_tableStopped);
	connect(model, &TableDataModel::errorOccured,
			this, &EnginePrivate::_q_tableErrorOccured);

	model->setupModel(name, watcher, connector, transformer);
	model->setLiveSyncEnabled(liveSync);

	tableMachines.insert(name, {machine, model});
	if (engineModel->isSyncActive()) {
		machine->start();
		model->start();
	}

	// notify external watcher
	Q_EMIT asyncWatcher->tableAdded(TableEvent{name, watcher->database().connectionName()});
}

void EnginePrivate::_q_tableRemoved(const QString &name)
{
	Q_Q(Engine);
	const auto watcher = qobject_cast<DatabaseWatcher*>(q->sender());
	Q_ASSERT(watcher);

	auto tm = tableMachines.take(name);
	if (tm.machine->isRunning())
		tm.model->exit();
	else
		tm.machine->deleteLater();

	// notify external watcher
	Q_EMIT asyncWatcher->tableRemoved(TableEvent{name, watcher->database().connectionName()});
}

void EnginePrivate::_q_tableStopped(const QString &table)
{
	Q_Q(Engine);
	// stopped after table remove -> delete the machine
	if (!tableMachines.contains(table)) {
		auto model = qobject_cast<TableDataModel*>(q->sender());
		Q_ASSERT(model);
		model->stateMachine()->deleteLater();
	}

	if (stopCache.remove(table) && stopCache.isEmpty())
		engineModel->allTablesStopped();
}

void EnginePrivate::_q_tableErrorOccured(const QString &table, const ErrorInfo &info)
{
	Q_Q(Engine);
	switch (info.type) {
	// global errors that need the engine to be stopped
	case Engine::ErrorType::Unknown:	// better safe then sorry
	case Engine::ErrorType::Network:	// network errors that are not connection breaks are globally critical
	case Engine::ErrorType::Database:	// affects the whole database -> stop syncing
	case Engine::ErrorType::Version:	// application is outdated and needs to be updated -> stop syncing
		engineModel->cancel(info);
		break;
	// local errors that only affect certain datasets, tables or even nothing
	case Engine::ErrorType::Table:
		// table errors disable sync for that table, but do NOT stop the engine
		if (const auto watcher = findWatcher(table); watcher)
			watcher->dropTable(table);
		Q_FALLTHROUGH();
	case Engine::ErrorType::Temporary:		// has automatic retry
	case Engine::ErrorType::Entry:			// has been marked corrupted
	case Engine::ErrorType::Transform:		// has been marked corrupted
		Q_EMIT q->errorOccured(info.type, info.data);
		break;
	// special errors
	case Engine::ErrorType::Transaction:
		// transaction errors are temporary -> restart table sync (but still report the error)
		if (const auto tm = tableMachines.value(table); tm.model) {
			tm.model->stop();
			tm.model->start();
		}
		Q_EMIT q->errorOccured(info.type, info.data);
		break;
	default:
		Q_UNREACHABLE();
	}
}



TableConfig::TableConfig(QString table, const QString &databaseConnection) :
	TableConfig{std::move(table), QSqlDatabase::database(databaseConnection, true)}
{}

TableConfig::TableConfig(QString table, QSqlDatabase database) :
	d{new TableConfigData{std::move(table), std::move(database)}}
{}

TableConfig::TableConfig(const TableConfig &other) = default;

TableConfig::TableConfig(TableConfig &&other) noexcept = default;

TableConfig &TableConfig::operator=(const TableConfig &other) = default;

TableConfig &TableConfig::operator=(TableConfig &&other) noexcept = default;

TableConfig::~TableConfig() = default;

void TableConfig::swap(TableConfig &other)
{
	d.swap(other.d);
}

QString TableConfig::table() const
{
	return d->table;
}

QSqlDatabase TableConfig::connection() const
{
	return d->connection;
}

QVersionNumber TableConfig::version() const
{
	return d->version;
}

bool TableConfig::isLiveSyncEnabled() const
{
	return d->liveSyncEnabled;
}

bool TableConfig::forceCreate() const
{
	return d->forceCreate;
}

QString TableConfig::primaryKey() const
{
	return d->primaryKey;
}

QStringList TableConfig::fields() const
{
	return d->fields;
}

QList<TableConfig::Reference> TableConfig::references() const
{
	return d->references;
}

void TableConfig::setVersion(QVersionNumber version)
{
	d->version = version;
}

void TableConfig::resetVersion()
{
	d->version = {};
}

void TableConfig::setLiveSyncEnabled(bool liveSyncEnabled)
{
	d->liveSyncEnabled = liveSyncEnabled;
}

void TableConfig::setForceCreate(bool forceCreate)
{
	d->forceCreate = forceCreate;
}

void TableConfig::setPrimaryKey(QString primaryKey)
{
	d->primaryKey = std::move(primaryKey);
}

void TableConfig::resetPrimaryKey()
{
	d->primaryKey.clear();
}

void TableConfig::setFields(QStringList fields)
{
	d->fields = std::move(fields);
}

void TableConfig::addField(const QString &field)
{
	d->fields.append(field);
}

void TableConfig::resetFields()
{
	d->fields.clear();
}

void TableConfig::setReferences(QList<TableConfig::Reference> references)
{
	d->references = std::move(references);
}

void TableConfig::addReference(const TableConfig::Reference &reference)
{
	d->references.append(reference);
}

void TableConfig::resetReferences()
{
	d->references.clear();
}



AsyncWatcherBackend::AsyncWatcherBackend(Engine *engine) :
	AsyncWatcherSource{engine}
{}

QList<TableEvent> AsyncWatcherBackend::activeTables() const
{
	const auto d = static_cast<Engine*>(parent())->d_func();
	QList<TableEvent> tables;
	for (const auto &watcher : qAsConst(d->watchers)) {
		const auto dbName = watcher->database().connectionName();
		for (const auto &table : watcher->tables())
			tables.append(TableEvent{table, dbName});
	}
	return tables;
}

void AsyncWatcherBackend::activate(const QString &name)
{
	// find the matching state machine and trigger and upload
	const auto d = static_cast<Engine*>(parent())->d_func();
	const auto [machine, model] = d->tableMachines.value(name);
	if (model)
		model->triggerExtUpload();
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
