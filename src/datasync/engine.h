#ifndef QTDATASYNC_ENGINE_H
#define QTDATASYNC_ENGINE_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"

#include <chrono>
#include <optional>

#include <QtCore/qobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qversionnumber.h>
#include <QtCore/qsettings.h>

#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlerror.h>

namespace QtDataSync {

class IAuthenticator;
class ICloudTransformer;
class TableSyncController;

namespace __private {
class SetupPrivate;
}

class TableConfigData;
class Q_DATASYNC_EXPORT TableConfig
{
	Q_GADGET

	Q_PROPERTY(QString table READ table CONSTANT)
	Q_PROPERTY(QSqlDatabase connection READ connection CONSTANT)

	Q_PROPERTY(QVersionNumber version READ version WRITE setVersion RESET resetVersion)

	Q_PROPERTY(bool liveSyncEnabled READ isLiveSyncEnabled WRITE setLiveSyncEnabled)
	Q_PROPERTY(bool forceCreate READ forceCreate WRITE setForceCreate)

	Q_PROPERTY(QString primaryKey READ primaryKey WRITE setPrimaryKey RESET resetPrimaryKey)
	Q_PROPERTY(QStringList fields READ fields WRITE setFields RESET resetFields)
	Q_PROPERTY(QList<Reference> references READ references WRITE setReferences RESET resetReferences)

public:
	using Reference = std::pair<QString, QString>;  // (table, field)

	TableConfig(QString table, const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	TableConfig(QString table, QSqlDatabase database);
	TableConfig(const TableConfig &other);
	TableConfig(TableConfig &&other) noexcept;
	TableConfig &operator=(const TableConfig &other);
	TableConfig &operator=(TableConfig &&other) noexcept;
	~TableConfig();

	void swap(TableConfig &other);
	friend inline void swap(TableConfig &lhs, TableConfig &rhs) {
		lhs.swap(rhs);
	}

	QString table() const;
	QSqlDatabase connection() const;
	QVersionNumber version() const;
	bool isLiveSyncEnabled() const;
	bool forceCreate() const;
	QString primaryKey() const;
	QStringList fields() const;
	QList<Reference> references() const;

	void setVersion(QVersionNumber version);
	void resetVersion();
	void setLiveSyncEnabled(bool liveSyncEnabled);
	void setForceCreate(bool forceCreate);
	void setPrimaryKey(QString primaryKey);
	void resetPrimaryKey();
	void setFields(QStringList fields);
	void addField(const QString &field);
	void resetFields();
	void setReferences(QList<Reference> references);
	void addReference(const Reference &reference);
	void resetReferences();

private:
	QSharedDataPointer<TableConfigData> d;
};

class EnginePrivate;
class Q_DATASYNC_EXPORT Engine : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QtDataSync::IAuthenticator* authenticator READ authenticator CONSTANT)
	Q_PROPERTY(QtDataSync::ICloudTransformer* transformer READ transformer CONSTANT)

	Q_PROPERTY(EngineState state READ state NOTIFY stateChanged)

	Q_PROPERTY(QStringList tables READ tables NOTIFY tablesChanged)

public:
	enum class DbSyncFlag {
		None = 0x00,
		ResyncTables = 0x01,
		SyncAllTables = 0x02,
		EnableLiveSync = 0x04,

		Default = ResyncTables
	};
	Q_DECLARE_FLAGS(DbSyncMode, DbSyncFlag)
	Q_FLAG(DbSyncMode)

	enum class ResyncFlag {
		Upload = 0x01,
		Download = 0x02,
		CheckLocalData = 0x04,

		CleanLocalData = (0x08 | Upload | CheckLocalData),
		ClearLocalData = (0x10 | Download),
		ClearServerData = (0x20 | Upload),

		SyncAll = (Upload | Download),
		ClearAll = (ClearLocalData | ClearServerData),
	};
	Q_DECLARE_FLAGS(ResyncMode, ResyncFlag)
	Q_FLAG(ResyncMode)

	enum class ErrorType {
		Temporary = 0,
		Network,
		Entry,
		Table,
		Database,
		Transaction,
		Transform,
		Version,

		Unknown = -1
	};
	Q_ENUM(ErrorType)

	enum class EngineState {
		Inactive,
		Error,
		SigningIn,
		DeletingAcc,
		TableSync,
		Stopping,

		Invalid = -1
	};
	Q_ENUM(EngineState)

	static const QString DefaultSettingsTableName;

	void syncDatabase(DbSyncMode dbSyncMode = DbSyncFlag::Default,
					  const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void syncDatabase(DbSyncMode dbSyncMode,
					  QSqlDatabase database);

	void syncTable(const QString &table,
				   bool enableLiveSync = false,
				   const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void syncTable(const QString &table,
				   bool enableLiveSync,
				   QSqlDatabase database);
	void syncTable(const TableConfig &config);

	void removeDatabaseSync(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
							bool deactivateSync = false);
	void removeDatabaseSync(QSqlDatabase database,
							bool deactivateSync = false);
	void removeTableSync(const QString &table);

	void unsyncDatabase(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void unsyncDatabase(QSqlDatabase database);
	void unsyncTable(const QString &table);

	void resyncDatabase(ResyncMode direction = ResyncFlag::SyncAll,
						const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void resyncDatabase(ResyncMode direction,
						QSqlDatabase database);
	void resyncTable(const QString &table,
					 ResyncMode direction);

	QSettings *syncSettings(QObject *parent = nullptr);
	QSettings *syncSettings(bool enableLiveSync,
							QObject *parent = nullptr);
	QSettings *syncSettings(bool enableLiveSync,
							const QString &databaseConnection,
							QObject *parent = nullptr);
	QSettings *syncSettings(bool enableLiveSync,
							QSqlDatabase database,
							QObject *parent = nullptr);
	QSettings *syncSettings(bool enableLiveSync,
							const QString &databaseConnection,
							const QString &tableName,
							QObject *parent = nullptr);
	QSettings *syncSettings(bool enableLiveSync,
							QSqlDatabase database,
							QString tableName,
							QObject *parent = nullptr);

	QSqlDatabase database(const QString &table) const;
	Q_INVOKABLE QtDataSync::TableSyncController *createController(QString table, QObject *parent = nullptr) const;

	IAuthenticator *authenticator() const;
	ICloudTransformer* transformer() const;
	EngineState state() const;
	QStringList tables() const;

	Q_INVOKABLE bool isRunning() const;
	bool waitForStopped(std::optional<std::chrono::milliseconds> timeout = std::nullopt) const;

public Q_SLOTS:
	void start();
	void stop();
	void logOut();
	void deleteAccount();

	void triggerSync(bool reconnectLiveSync = false);
	void setLiveSyncEnabled(bool liveSyncEnabled);

Q_SIGNALS:
	void errorOccured(Engine::ErrorType type, const QVariant &errorData, QPrivateSignal = {});

	void stateChanged(Engine::EngineState state, QPrivateSignal = {});
	void tablesChanged(const QStringList &tables, QPrivateSignal = {});

private:
	friend class __private::SetupPrivate;
	friend class AsyncWatcherBackend;
	friend class EngineThread;
	Q_DECLARE_PRIVATE(Engine)

	Q_PRIVATE_SLOT(d_func(), void _q_startTableSync())
	Q_PRIVATE_SLOT(d_func(), void _q_stopTableSync())
	Q_PRIVATE_SLOT(d_func(), void _q_errorOccured(const EnginePrivate::ErrorInfo &))
	Q_PRIVATE_SLOT(d_func(), void _q_tableAdded(const QString &, bool))
	Q_PRIVATE_SLOT(d_func(), void _q_tableRemoved(const QString &))
	Q_PRIVATE_SLOT(d_func(), void _q_tableStopped(const QString &))
	Q_PRIVATE_SLOT(d_func(), void _q_tableErrorOccured(const QString &, const EnginePrivate::ErrorInfo &))

	explicit Engine(QScopedPointer<__private::SetupPrivate> &&setup, QObject *parent = nullptr);
};

class Q_DATASYNC_EXPORT TableException : public Exception
{
public:
	TableException(QString table, QString message, QSqlError error);

	QString qWhat() const override;
	QString message() const;
	QString table() const;
	QSqlError sqlError() const;

	void raise() const override;
	ExceptionBase *clone() const override;

protected:
	QString _table;
	QString _message;
	QSqlError _error;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(QtDataSync::Engine::ResyncMode)
Q_DECLARE_OPERATORS_FOR_FLAGS(QtDataSync::Engine::DbSyncMode)

Q_DECLARE_METATYPE(QSqlError)

#endif // QTDATASYNC_ENGINE_H
