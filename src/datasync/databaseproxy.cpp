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
		connect(watcher, &DatabaseWatcher::triggerSync,
				this, &DatabaseProxy::markTableDirty);
		return watcher;
	}
}

void DatabaseProxy::dropWatcher(QSqlDatabase &&database)
{
	auto watcher = _watchers.take(database.connectionName());
	if (watcher)
		watcher->deleteLater();
}

bool DatabaseProxy::clearDirtyTable(const QString &name, bool headOnly)
{
	if (headOnly) {
		if (_dirtyTables.head() == name) {
			_dirtyTables.dequeue();
			return true;
		} else
			return false;
	} else
		return _dirtyTables.removeOne(name);
}

bool DatabaseProxy::hasDirtyTables() const
{
	return !_dirtyTables.isEmpty();
}

QString DatabaseProxy::nextDirtyTable() const
{
	return _dirtyTables.isEmpty() ? QString{} : _dirtyTables.head();
}

void DatabaseProxy::fillDirtyTables()
{
	_dirtyTables.clear();
	for (auto watcher : qAsConst(_watchers))
		_dirtyTables.append(watcher->tables());
	if (const auto dupes = _dirtyTables.removeDuplicates(); dupes > 0) {
		qCWarning(logDbProxy) << "Detected" << dupes
							  << "tables with identical names - this can lead to synchronization failures!";
	}
	Q_EMIT triggerSync();
}

void DatabaseProxy::markTableDirty(const QString &name)
{
	if (name.isEmpty())
		fillDirtyTables();
	else if (!_dirtyTables.contains(name)) {
		_dirtyTables.enqueue(name);
		Q_EMIT triggerSync();
	}
}

void DatabaseProxy::tableAdded(const QString &name)
{
	Q_ASSERT(qobject_cast<DatabaseWatcher*>(sender()));
	_tableMap.insert(name, static_cast<DatabaseWatcher*>(sender()));
}

void DatabaseProxy::tableRemoved(const QString &name)
{
	Q_ASSERT(qobject_cast<DatabaseWatcher*>(sender()));
	_tableMap.remove(name);
}
