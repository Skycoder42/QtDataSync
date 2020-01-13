#ifndef QTDATASYNC_ENGINE_H
#define QTDATASYNC_ENGINE_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"

#include <QtCore/qobject.h>

#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlerror.h>

namespace QtDataSync {

class FirebaseAuthenticator;
class ICloudTransformer;

namespace __private {
class SetupPrivate;
}

class EnginePrivate;
class Q_DATASYNC_EXPORT Engine : public QObject
{
	Q_OBJECT

	Q_PROPERTY(FirebaseAuthenticator* authenticator READ authenticator CONSTANT)
	Q_PROPERTY(ICloudTransformer* transformer READ transformer CONSTANT)

	Q_PROPERTY(EngineState state READ state NOTIFY stateChanged)

public:
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

	enum class ErrorType {
		Network = 0,
		LiveSyncSoft,
		LiveSyncHard,
		Entry,
		Table,
		Database,
		Transaction,
		Transform,

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

	enum class TableState {
		Inactive,
		Error,
		Initializing,
		LiveSync,
		Downloading,
		Uploading,
		Synchronized,

		Invalid = -1
	};
	Q_ENUM(TableState)

	void syncDatabase(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
					  bool autoActivateSync = true,
					  bool enableLiveSync = false,
					  bool addAllTables = false);
	void syncDatabase(QSqlDatabase database,
					  bool autoActivateSync = true,
					  bool enableLiveSync = false,
					  bool addAllTables = false);

	void syncTable(const QString &table,
				   bool enableLiveSync = false,
				   const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
				   const QStringList &fields = {},
				   const QString &primaryKeyType = {});
	void syncTable(const QString &table,
				   bool enableLiveSync,
				   QSqlDatabase database,
				   const QStringList &fields = {},
				   const QString &primaryKeyType = {});

	void removeDatabaseSync(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
							bool deactivateSync = false);
	void removeDatabaseSync(QSqlDatabase database,
							bool deactivateSync = false);
	void removeTableSync(const QString &table,
						 const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void removeTableSync(const QString &table,
						 QSqlDatabase database);

	void unsyncDatabase(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void unsyncDatabase(QSqlDatabase database);
	void unsyncTable(const QString &table,
					 const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void unsyncTable(const QString &table,
					 QSqlDatabase database);

	void resyncDatabase(ResyncMode direction = ResyncFlag::SyncAll,
						const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void resyncDatabase(ResyncMode direction,
						QSqlDatabase database);
	void resyncTable(const QString &table,
					 ResyncMode direction = ResyncFlag::SyncAll,
					 const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void resyncTable(const QString &table,
					 ResyncMode direction,
					 QSqlDatabase database);

	bool isLiveSyncEnabled(const QString &table) const;
	TableState tableState(const QString &table) const;

	FirebaseAuthenticator *authenticator() const;
	ICloudTransformer* transformer() const;
	EngineState state() const;

public Q_SLOTS:
	void start();
	void stop();
	void logOut();
	void deleteAccount();

	void triggerSync(bool reconnectLiveSync = false);

	void setLiveSyncEnabled(bool liveSyncEnabled);
	void setLiveSyncEnabled(const QString &table,
							bool liveSyncEnabled);

Q_SIGNALS:
	void liveSyncEnabledChanged(const QString &table, bool liveSyncEnabled, QPrivateSignal = {});
	void errorOccured(ErrorType type,
					  const QString &errorMessage,
					  const QVariant &errorData,
					  QPrivateSignal = {});

	void tableStateChanged(const QString &table, TableState state, QPrivateSignal = {});
	void stateChanged(EngineState state, QPrivateSignal = {});

private:
	friend class __private::SetupPrivate;
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

Q_DECLARE_METATYPE(QSqlError)

#endif // QTDATASYNC_ENGINE_H
