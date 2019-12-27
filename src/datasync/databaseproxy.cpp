#include "databaseproxy_p.h"
using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logDbProxy, "qt.datasync.DatabaseProxy")

DatabaseProxy::DatabaseProxy(Engine *engine) :
	QObject{engine}
{}

DatabaseWatcher *DatabaseProxy::watcher(QSqlDatabase &&database)
{
	const auto dbName = database.connectionName();
	auto wIter = _watchers.find(dbName);
	if (wIter != _watchers.end())
		return *wIter;
	else {
		auto watcher = new DatabaseWatcher{std::move(database), this};
		_watchers.insert(dbName, watcher);
		connect(watcher, &DatabaseWatcher::tableAdded,
				this, &DatabaseProxy::tableAdded);
		connect(watcher, &DatabaseWatcher::tableRemoved,
				this, &DatabaseProxy::tableRemoved);
//		connect(watcher, &DatabaseWatcher::triggerSync,
//				this, &DatabaseProxy::markTableDirty);  // TODO change
		return watcher;
	}
}

void DatabaseProxy::dropWatcher(QSqlDatabase &&database)
{
	auto watcher = _watchers.take(database.connectionName());
	if (watcher)
		watcher->deleteLater();
}

void DatabaseProxy::clearDirtyTable(const QString &name, Type type)
{
	_tables[name].state.setFlag(toState(type), false);
}

std::optional<QString> DatabaseProxy::nextDirtyTable(Type type) const
{
	const auto nextIt = std::find_if(_tables.begin(), _tables.end(), [flag = toState(type)](const TableInfo &info){
		return info.state.testFlag(flag);
	});
	if (nextIt != _tables.end())
		return nextIt.key();
	else
		return std::nullopt;
}

QDateTime DatabaseProxy::lastSync(const QString &name) const
{
	Q_UNIMPLEMENTED();
}

void DatabaseProxy::fillDirtyTables(Type type)
{
	const auto flag = toState(type);
	for (auto &info : _tables)
		info.state.setFlag(flag);
	Q_EMIT triggerSync();
}

void DatabaseProxy::markTableDirty(const QString &name, Type type)
{
	if (name.isEmpty())
		fillDirtyTables(type);
	else {
		auto &info = _tables[name];
		const auto flag = toState(type);
		if (!info.state.testFlag(flag)) {
			_tables[name].state.setFlag(flag);
			Q_EMIT triggerSync();
		}
	}
}

void DatabaseProxy::tableAdded(const QString &name)
{
	Q_ASSERT(qobject_cast<DatabaseWatcher*>(sender()));
	_tables.insert(name, static_cast<DatabaseWatcher*>(sender()));
}

void DatabaseProxy::tableRemoved(const QString &name)
{
	Q_ASSERT(qobject_cast<DatabaseWatcher*>(sender()));
	_tables.remove(name);
}

DatabaseProxy::TableStateFlag DatabaseProxy::toState(DatabaseProxy::Type type)
{
	switch (type) {
	case Type::Local:
		return TableStateFlag::LocalDirty;
	case Type::Cloud:
		return TableStateFlag::CloudDirty;
	case Type::Both:
		return TableStateFlag::AllDirty;
	}
}
