#include "asyncwatcher.h"
#include "asyncwatcher_p.h"
#include "engine_p.h"
#include <QtSql/QSqlDriver>
using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logAsyncWatcher, "qt.datasync.AsyncWatcher")

AsyncWatcher::AsyncWatcher(Engine *engine, QObject *parent) :
	QObject{*new AsyncWatcherPrivate{}, parent}
{
	Q_D(AsyncWatcher);
	d->engine = engine;
	d->initForEngine();
}

AsyncWatcher::AsyncWatcher(const QUrl &remoteUrl, QObject *parent) :
	QObject{*new AsyncWatcherPrivate{}, parent}
{
	Q_D(AsyncWatcher);
	d->roNode = new QRemoteObjectNode{this};
	if (!d->roNode->connectToNode(remoteUrl)) {
		qCCritical(logAsyncWatcher) << "Failed to connect to remote node with error:"
									<< d->roNode->lastError();
	} else
		d->initForRemote();
}

AsyncWatcher::AsyncWatcher(QRemoteObjectNode *remoteNode, QObject *parent) :
	QObject{*new AsyncWatcherPrivate{}, parent}
{
	Q_D(AsyncWatcher);
	d->roNode = remoteNode;
	d->initForRemote();
}

void AsyncWatcher::addDatabase(const QString &databaseConnection)
{
	addDatabase(QSqlDatabase::database(databaseConnection, true));
}

void AsyncWatcher::addDatabase(const QString &databaseConnection, const QString &originalConnection)
{
	addDatabase(QSqlDatabase::database(databaseConnection, true),
				originalConnection);
}

void AsyncWatcher::addDatabase(QSqlDatabase database)
{
	Q_D(AsyncWatcher);
	d->unnamedConnections.append({
		database,
		QObjectPrivate::connect(database.driver(), qOverload<const QString &>(&QSqlDriver::notification),
								d, &AsyncWatcherPrivate::_q_tableEvent)
	});
	for (const auto &tInfo : qAsConst(d->allTables))
		d->watch(database, tInfo.table());
}

void AsyncWatcher::addDatabase(QSqlDatabase database, const QString &originalConnection)
{
	Q_D(AsyncWatcher);
	d->namedConnections.insert(originalConnection, {
		database,
		QObjectPrivate::connect(database.driver(), qOverload<const QString &>(&QSqlDriver::notification),
								d, &AsyncWatcherPrivate::_q_tableEvent)
	});
	for (const auto &tInfo : qAsConst(d->allTables)) {
		if (tInfo.connection() == originalConnection)
			d->watch(database, tInfo.table());
	}
}

void AsyncWatcher::removeDatabase(const QString &databaseConnection)
{
	Q_D(AsyncWatcher);
	for (auto it = d->namedConnections.begin(), end = d->namedConnections.end(); it != end; ++it) {
		if (it->db.connectionName() == databaseConnection) {
			d->unwatchAll(it->db, it.key());
			disconnect(it->notifyCon);
			d->namedConnections.erase(it);
			return;
		}
	}

	for (auto it = d->unnamedConnections.begin(), end = d->unnamedConnections.end(); it != end; ++it) {
		if (it->db.connectionName() == databaseConnection) {
			d->unwatchAll(it->db);
			disconnect(it->notifyCon);
			d->unnamedConnections.erase(it);
			return;
		}
	}
}

void AsyncWatcher::removeDatabase(QSqlDatabase database)
{
	removeDatabase(database.connectionName());
}

void AsyncWatcherPrivate::initForEngine()
{
	Q_Q(AsyncWatcher);
	Q_ASSERT(engine);
	backend = EnginePrivate::obtainAWB(engine);
	connect(backend, &AsyncWatcherBackend::tableAdded,
			this, &AsyncWatcherPrivate::_q_tableAdded,
			Qt::QueuedConnection);
	connect(backend, &AsyncWatcherBackend::tableRemoved,
			this, &AsyncWatcherPrivate::_q_tableRemoved,
			Qt::QueuedConnection);

	QList<TableEvent> aTables;
	const auto conType = backend->thread() == QThread::currentThread() ?
		Qt::DirectConnection :
		Qt::BlockingQueuedConnection;
	if (QMetaObject::invokeMethod(backend, "activeTables",
								  conType,
								  Q_RETURN_ARG(QList<TableEvent>, aTables))) {
		for (const auto &tEvent : qAsConst(aTables))
			_q_tableAdded(tEvent);
		QMetaObject::invokeMethod(q, "initialized", Qt::QueuedConnection);
	} else
		qCCritical(logAsyncWatcher) << "Failed to obtain initial table list from engine";
}

void AsyncWatcherPrivate::initForRemote()
{
	Q_Q(AsyncWatcher);
	Q_ASSERT(roNode);
	connect(roNode, &QRemoteObjectNode::error,
			this, &AsyncWatcherPrivate::_q_nodeError);
	rep = roNode->acquire<AsyncWatcherReplica>();
	rep->setParent(q);

	connect(rep, &AsyncWatcherReplica::tableAdded,
			this, &AsyncWatcherPrivate::_q_tableAdded);
	connect(rep, &AsyncWatcherReplica::tableRemoved,
			this, &AsyncWatcherPrivate::_q_tableRemoved);
	connect(rep, &AsyncWatcherReplica::initialized,
			this, &AsyncWatcherPrivate::_q_nodeInitialized);
	if (rep->isInitialized())
		_q_nodeInitialized();
}

void AsyncWatcherPrivate::_q_nodeError(QRemoteObjectNode::ErrorCode error)
{
	qCCritical(logAsyncWatcher) << "An remote object node error occured."
								<< "The async watcher might not be operational anymore."
								<< "Error code:" << error;
}

void AsyncWatcherPrivate::_q_nodeInitialized()
{
	Q_Q(AsyncWatcher);
	for (const auto &tInfo : rep->activeTables())
		_q_tableAdded(tInfo);
	QMetaObject::invokeMethod(q, "initialized", Qt::QueuedConnection);
}

void AsyncWatcherPrivate::_q_tableAdded(const TableEvent &event)
{
	qCDebug(logAsyncWatcher) << "Watching table" << event.table()
							 << "of original connection" << event.connection();
	allTables.append(event);
	const auto namedIt = namedConnections.find(event.connection());
	if (namedIt != namedConnections.end())
		watch(namedIt->db, event.table());
	for (auto &db : unnamedConnections)
		watch(db.db, event.table());
}

void AsyncWatcherPrivate::_q_tableRemoved(const TableEvent &event)
{
	qCDebug(logAsyncWatcher) << "Stopping table watch for" << event.table()
							 << "of original connection" << event.connection();
	allTables.removeOne(event);
	const auto namedIt = namedConnections.find(event.connection());
	if (namedIt != namedConnections.end())
		unwatch(namedIt->db, event.table());
	for (auto &db : unnamedConnections)
		unwatch(db.db, event.table());
}

void AsyncWatcherPrivate::_q_tableEvent(const QString &name)
{
	if (backend) {
		QMetaObject::invokeMethod(backend, "activate",
								  Qt::QueuedConnection,
								  Q_ARG(QString, name));
	} else if (rep)
		rep->activate(name);
}

void AsyncWatcherPrivate::watch(QSqlDatabase &db, const QString &tableName)
{
	if (!db.driver()->subscribeToNotification(tableName)) {
		qCWarning(logAsyncWatcher) << "Failed to watch table" << tableName
								   << "on connection" << db.connectionName();
	}
}

void AsyncWatcherPrivate::unwatch(QSqlDatabase &db, const QString &tableName)
{
	if (!db.driver()->unsubscribeFromNotification(tableName)) {
		qCWarning(logAsyncWatcher) << "Failed to unwatch table" << tableName
								   << "on connection" << db.connectionName();
	}
}

void AsyncWatcherPrivate::unwatchAll(QSqlDatabase &db, const QString &conName)
{
	if (conName.isEmpty()) {
		for (const auto &tEvent : qAsConst(allTables))
			unwatch(db, tEvent.table());
	} else {
		for (const auto &tEvent : qAsConst(allTables)) {
			if (tEvent.connection() == conName)
				unwatch(db, tEvent.table());
		}
	}
}

#include "moc_asyncwatcher.cpp"
