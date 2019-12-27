#ifndef QTDATASYNC_DATABASEPROXY_H
#define QTDATASYNC_DATABASEPROXY_H

#include "qtdatasync_global.h"
#include "engine.h"
#include "databasewatcher_p.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QHash>
#include <QtCore/QQueue>
#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>

namespace QtDataSync {

class Q_DATASYNC_EXPORT DatabaseProxy : public QObject
{
	Q_OBJECT

public:
	enum class Type {
		Local,
		Cloud,
		Both
	};
	Q_ENUM(Type)
	enum class TableStateFlag {
		Clean = 0x00,
		LocalDirty = 0x01,
		CloudDirty = 0x02,
		Blocked = 0x04,

		AllDirty = (LocalDirty | CloudDirty)
	};
	Q_DECLARE_FLAGS(TableState, TableStateFlag)
	Q_ENUM(TableState)

	explicit DatabaseProxy(Engine *engine = nullptr);

	DatabaseWatcher *watcher(QSqlDatabase &&database);  // gets or creates a watcher for the given database connection
	void dropWatcher(QSqlDatabase &&database);  // drops a watcher without removing synced tables

	// table management
	void clearDirtyTable(const QString &name, Type type);
	std::optional<QString> nextDirtyTable(Type type) const;

	QDateTime lastSync(const QString &name) const;

public Q_SLOTS:
	void fillDirtyTables(Type type);
	void markTableDirty(const QString &name, Type type);

Q_SIGNALS:
	void triggerSync(QPrivateSignal = {});
	void databaseError(const QString &errorString, QPrivateSignal = {});

private Q_SLOTS:
	void tableAdded(const QString &name);
	void tableRemoved(const QString &name);

private:
	struct TableInfo {
		QPointer<DatabaseWatcher> watcher;
		TableState state = TableStateFlag::Clean;

		inline TableInfo(DatabaseWatcher *watcher = nullptr) :
			watcher{watcher}
		{}
	};
	// TODO watcher ref count to remove on last table removed
	// TODO remove watcher on database removed / closed?

	QHash<QString, DatabaseWatcher*> _watchers;  // dbName -> watcher
	QHash<QString, TableInfo> _tables;  // tableName -> state

	static TableStateFlag toState(Type type);
};

Q_DECLARE_LOGGING_CATEGORY(logDbProxy)

}

Q_DECLARE_OPERATORS_FOR_FLAGS(QtDataSync::DatabaseProxy::TableState)

#endif // QTDATASYNC_DATABASEPROXY_H
