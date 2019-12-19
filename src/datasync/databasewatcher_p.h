#ifndef QTDATASYNC_DATABASEWATCHER_H
#define QTDATASYNC_DATABASEWATCHER_H

#include "qtdatasync_global.h"

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlField>
#include <QtSql/QSqlDriver>

namespace QtDataSync {

class ExQuery : public QSqlQuery
{
public:
	ExQuery(QSqlDatabase db);

	void prepare(const QString &query);
	void exec();
	void exec(const QString &query);
};

class ExTransaction
{
	Q_DISABLE_COPY(ExTransaction)

public:
	ExTransaction();
	ExTransaction(QSqlDatabase db);
	ExTransaction(ExTransaction &&other) noexcept;
	ExTransaction &operator=(ExTransaction &&other) noexcept;
	~ExTransaction();

	void commit();
	void rollback();

private:
	std::optional<QSqlDatabase> _db;
};

// TODO add public thread sync watcher, that just watches on different threads

class Q_DATASYNC_EXPORT DatabaseWatcher : public QObject
{
	Q_OBJECT

public:
	enum ChangeState {
		Unchanged = 0,
		Changed = 1,
		Deleted = 2
	};
	Q_ENUM(ChangeState)

	static const QString MetaTable;
	static const QString TablePrefix;

	explicit DatabaseWatcher(QSqlDatabase &&db, QObject *parent = nullptr);

	QSqlDatabase database() const;

	// table setup
	bool hasTables() const;
	QStringList tables() const;

	bool addAllTables(QSql::TableType type = QSql::Tables);
	bool addTable(const QString &name, const QStringList &fields = {}, QString primaryType = {});

	void removeAllTables();
	void removeTable(const QString &name, bool removeRef = true);

	void unsyncAllTables();
	void unsyncTable(const QString &name, bool removeRef = true);

	// sync
	quint64 changeCount() const; // get current change count for all registered tables
	bool processChanges(const QString &tableName,
						const std::function<void(QVariant, quint64, ChangeState)> &callback,
						int limit = 100);

Q_SIGNALS:
	void triggerSync(const QString &tableName, QPrivateSignal);

private Q_SLOTS:
	void dbNotify(const QString &name);

private:
	QSqlDatabase _db;

	QHash<QString, QStringList> _tables;

	QString sqlTypeName(const QSqlField &field) const;
};

Q_DECLARE_LOGGING_CATEGORY(logDbWatcher)

}

#endif // QTDATASYNC_DATABASEWATCHER_H
