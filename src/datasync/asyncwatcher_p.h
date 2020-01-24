#ifndef QTDATASYNC_ASYNCWATCHER_P_H
#define QTDATASYNC_ASYNCWATCHER_P_H

#include "asyncwatcher.h"
#include "engine.h"

#include <QtCore/QPointer>
#include <QtCore/QLoggingCategory>

#include <QtRemoteObjects/QRemoteObjectNode>

#include "rep_asyncwatcher_merged.h"
#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class AsyncWatcherBackend;

class Q_DATASYNC_EXPORT AsyncWatcherPrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(AsyncWatcher)

public:
	struct DbInfo {
		QSqlDatabase db;
		QMetaObject::Connection notifyCon;
	};

	QPointer<Engine> engine;
	QPointer<QRemoteObjectNode> roNode;

	AsyncWatcherReplica *rep = nullptr;
	QPointer<AsyncWatcherBackend> backend;

	QHash<QString, DbInfo> namedConnections;
	QList<DbInfo> unnamedConnections;
	QList<std::pair<QString, QString>> allTables;

	void initForEngine();
	void initForRemote();

	void _q_nodeError(QRemoteObjectNode::ErrorCode error);
	void _q_nodeInitialized();

	void _q_tableAdded(const QString &name, const QString &connection);
	void _q_tableRemoved(const QString &name, const QString &connection);

	void _q_tableEvent(const QString &name);

	void watch(QSqlDatabase &db, const QString &tableName);
	void unwatch(QSqlDatabase &db, const QString &tableName);
	void unwatchAll(QSqlDatabase &db, const QString &conName = {});
};

Q_DECLARE_LOGGING_CATEGORY(logAsyncWatcher)

}

#endif // QTDATASYNC_ASYNCWATCHER_P_H
