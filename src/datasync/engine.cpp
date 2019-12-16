#include "engine.h"
#include "engine_p.h"
using namespace QtDataSync;

IAuthenticator *Engine::authenticator() const
{
	Q_D(const Engine);
	return d->setup->authenticator;
}

bool Engine::registerForSync(const QString &databaseConnection, const QString &table, const QStringList &fields)
{
	return registerForSync(QSqlDatabase::database(databaseConnection, true), table, fields);
}

bool Engine::registerForSync(QSqlDatabase database, const QString &table, const QStringList &fields)
{
	Q_D(Engine);
	if (!database.isOpen())
		return false;
	auto watcher = new DatabaseWatcher{std::move(database), new SqliteTypeConverter{}, this};
	watcher->addTable(table);
	d->dbWatchers.insert(table, watcher);
	return true;
}

bool Engine::removeFromSync(const QString &databaseConnection, const QString &table)
{
	return false;
}

void Engine::start()
{
	Q_D(Engine);
	QMetaObject::invokeMethod(d->setup->authenticator, "signIn", Qt::QueuedConnection);
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
