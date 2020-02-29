#ifndef QTDATASYNC_DATABASEWATCHER_H
#define QTDATASYNC_DATABASEWATCHER_H

#include "qtdatasync_global.h"
#include "cloudtransformer.h"
#include "exception.h"
#include "engine.h"
#include "setup_impl.h"

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlField>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>

namespace QtDataSync {

class ExTransaction;

class Q_DATASYNC_EXPORT DatabaseWatcher : public QObject
{
	Q_OBJECT

public:
	using DatabaseConfig = __private::SetupPrivate::DatabaseConfig;

	enum class ErrorScope {
		Entry = static_cast<int>(Engine::ErrorType::Entry),
		Table = static_cast<int>(Engine::ErrorType::Table),
		Database = static_cast<int>(Engine::ErrorType::Database),
		Transaction = static_cast<int>(Engine::ErrorType::Transaction),
		Version = static_cast<int>(Engine::ErrorType::Version)
	};
	Q_ENUM(ErrorScope)

	enum class TableState {
		Inactive = 0,
		Active = 1
	};
	Q_ENUM(TableState)

	enum class ChangeState {
		Unchanged = 0,
		Changed = 1,
		Corrupted = 2
	};
	Q_ENUM(ChangeState)

	static const QString MetaTable;
	static const QString FieldTable;
	static const QString ReferenceTable;
	static const QString TablePrefix;

	explicit DatabaseWatcher(QSqlDatabase &&db,
							 DatabaseConfig config,
							 QObject *parent = nullptr);

	QSqlDatabase database() const;

	// setup functions
	bool hasTables() const;
	QStringList tables() const;
	bool hasTable(const QString &name) const;

	void reactivateTables(bool liveSync);
	void addAllTables(bool liveSync, QSql::TableType type = QSql::Tables);
	void addTable(const TableConfig &config);

	void dropAllTables();
	void dropTable(const QString &name, bool removeRef = true);

	void removeAllTables();
	void removeTable(const QString &name, bool removeRef = true);

	void unsyncAllTables();
	void unsyncTable(const QString &name, bool removeRef = true);

	void resyncAllTables(Engine::ResyncMode direction);
	void resyncTable(const QString &name, Engine::ResyncMode direction);

	// sync functions
	std::optional<QDateTime> lastSync(const QString &tableName);
	bool shouldStore(const DatasetId &key,
					 const CloudData &data);
	void storeData(const LocalData &data);
	std::optional<LocalData> loadData(const QString &name);
	void markUnchanged(const DatasetId &key, const QDateTime &modified, bool deleted);
	void markCorrupted(const DatasetId &key, const QDateTime &modified);

Q_SIGNALS:
	void tableAdded(const QString &tableName, bool liveSync, QPrivateSignal = {});
	void tableRemoved(const QString &tableName, QPrivateSignal = {});
	void triggerSync(const QString &tableName, QPrivateSignal = {});
	void triggerResync(const QString &tableName, bool deleteTable, QPrivateSignal = {});

	void databaseError(ErrorScope scope,
					   const QVariant &key,
					   const QSqlError &sqlError,
					   QPrivateSignal = {});

private Q_SLOTS:
	void dbNotify(const QString &name);

private:
	friend class SqlException;

	using PKeyInfo = std::pair<QString, int>;

	struct TableInfo {
		std::optional<QVersionNumber> version;
		PKeyInfo pKeyInfo;
		QStringList fields;
		QList<TableConfig::Reference> references;

		inline TableInfo() = default;

		inline TableInfo(std::optional<QVersionNumber> &&version,
						 PKeyInfo &&pKeyInfo,
						 QStringList &&fields,
						 QList<TableConfig::Reference> &&references) :
			version{std::move(version)},
			pKeyInfo{std::move(pKeyInfo)},
			fields{std::move(fields)},
			references{std::move(references)}
		{}
	};

	QSqlDatabase _db;
	DatabaseConfig _config;
	QHash<QString, TableInfo> _tables;

	QString sqlTypeName(const QSqlField &field) const;
	QString tableName(const QString &table, bool asSyncTable = false) const;
	QString fieldName(const QString &field) const;

	void createMetaTables(const QString &table);
	PKeyInfo createSyncTables(const TableConfig &config);

	void updateLastSync(const QString &table, const QDateTime &uploaded);
	bool shouldStoreImpl(const LocalData &data, const QVariant &escKey);
	void verifyVersion(const TableInfo &tInfo, const LocalData &data);

	QString encodeKey(const QVariant &key, int type) const;
	QVariant decodeKey(const QString &key, int type) const;
};

class Q_DATASYNC_EXPORT SqlException : public Exception
{
public:
	using ErrorScope = DatabaseWatcher::ErrorScope;

	SqlException(ErrorScope scope,
				 QVariant key,
				 QSqlError error);

	ErrorScope scope() const;
	QVariant key() const;
	QSqlError error() const;

	void raise() const override;
	ExceptionBase *clone() const override;
	QString qWhat() const override;

	void emitFor(DatabaseWatcher *watcher) const;

protected:
	ErrorScope _scope;
	QVariant _key;
	QSqlError _error;
};

class Q_DATASYNC_EXPORT InvalidVersionException : public SqlException
{
public:
	InvalidVersionException(const QString &table, QVersionNumber local, QVersionNumber remote);

	QString qWhat() const override;
	void raise() const override;
	ExceptionBase *clone() const override;

protected:
	QVersionNumber _local;
	QVersionNumber _remote;
};

class Q_DATASYNC_EXPORT ExQuery : public QSqlQuery
{
public:
	using ErrorScope = DatabaseWatcher::ErrorScope;

	ExQuery(QSqlDatabase db, ErrorScope scope, QVariant key);

	void prepare(const QString &query);
	void exec();
	void exec(const QString &query);

private:
	ErrorScope _scope;
	QVariant _key;
};

class Q_DATASYNC_EXPORT ExTransaction
{
	Q_DISABLE_COPY(ExTransaction)

public:
	using ErrorScope = DatabaseWatcher::ErrorScope;

	ExTransaction();
	ExTransaction(QSqlDatabase db, TransactionMode mode, QVariant key);
	ExTransaction(ExTransaction &&other) noexcept;
	ExTransaction &operator=(ExTransaction &&other) noexcept;
	~ExTransaction();

	void commit();
	void rollback();

private:
	std::optional<QSqlDatabase> _db;
	QVariant _key;
};

Q_DECLARE_LOGGING_CATEGORY(logDbWatcher)

}

#endif // QTDATASYNC_DATABASEWATCHER_H
