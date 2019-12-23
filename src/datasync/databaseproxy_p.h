#ifndef QTDATASYNC_DATABASEPROXY_H
#define QTDATASYNC_DATABASEPROXY_H

#include "qtdatasync_global.h"
#include "engine.h"
#include "databasewatcher_p.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QHash>
#include <QtCore/QQueue>
#include <QtCore/QLoggingCategory>

namespace QtDataSync {

class Q_DATASYNC_EXPORT DatabaseProxy : public QObject
{
	Q_OBJECT

public:
	explicit DatabaseProxy(Engine *engine = nullptr);

	DatabaseWatcher *watcher(QSqlDatabase &&database);  // gets or creates a watcher for the given database connection
	void dropWatcher(QSqlDatabase &&database);  // drops a watcher without removing synced tables

	// table management
	bool clearDirtyTable(const QString &name, bool headOnly = true);
	bool hasDirtyTables() const;
	QString nextDirtyTable() const;

public Q_SLOTS:
	void fillDirtyTables();
	void markTableDirty(const QString &name);

Q_SIGNALS:
	void triggerSync(QPrivateSignal = {});
	void databaseError(const QString &errorString, QPrivateSignal = {});

private Q_SLOTS:
	void tableAdded(const QString &name);
	void tableRemoved(const QString &name);

private:
	// TODO watcher ref count to remove on last table removed
	// TODO remove watcher on database removed / closed?

	QHash<QString, DatabaseWatcher*> _watchers;  // dbName -> watcher
	QQueue<QString> _dirtyTables;

	QHash<QString, QPointer<DatabaseWatcher>> _tableMap;  // tableName -> watcher
};

Q_DECLARE_LOGGING_CATEGORY(logDbProxy)

}

#endif // QTDATASYNC_DATABASEPROXY_H
