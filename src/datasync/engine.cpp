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
	if (!d->setup->ntpAddress.isEmpty()) {
		d->ntpSync = new NtpSync{this};
		d->ntpSync->syncWith(d->setup->ntpAddress, d->setup->ntpPort);
	}
#endif

	// start synchronization
	QMetaObject::invokeMethod(d->setup->authenticator, "signIn", Qt::QueuedConnection);
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
}



const SetupPrivate *EnginePrivate::setupFor(const Engine *engine)
{
	return engine->d_func()->setup.data();
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
