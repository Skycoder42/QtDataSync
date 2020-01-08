#ifndef QTDATASYNC_ENGINE_H
#define QTDATASYNC_ENGINE_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"

#include <QtCore/qobject.h>

#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlerror.h>

namespace QtDataSync {

class IAuthenticator;
class ICloudTransformer;

class SetupPrivate;

class EnginePrivate;
class Q_DATASYNC_EXPORT Engine : public QObject
{
	Q_OBJECT

	Q_PROPERTY(IAuthenticator* authenticator READ authenticator CONSTANT)
	Q_PROPERTY(ICloudTransformer* transformer READ transformer CONSTANT)

	Q_PROPERTY(bool liveSyncEnabled READ isLiveSyncEnabled WRITE setLiveSyncEnabled NOTIFY liveSyncEnabledChanged)

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
		Network,
		LiveSync,
		Entry,
		Table,
		Database,
		System
	};
	Q_ENUM(ErrorType)

	void syncDatabase(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
					  bool autoActivateSync = true,
					  bool addAllTables = false);
	void syncDatabase(QSqlDatabase database,
					  bool autoActivateSync = true,
					  bool addAllTables = false);

	void syncTable(const QString &table,
				   const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
				   const QStringList &fields = {},
				   const QString &primaryKeyType = {});
	void syncTable(const QString &table,
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

	IAuthenticator *authenticator() const;
	ICloudTransformer* transformer() const;
	bool isLiveSyncEnabled() const;

public Q_SLOTS:
	void start();
	void stop();
	void logOut();
	void deleteAccount();

	void triggerSync();

	void setLiveSyncEnabled(bool liveSyncEnabled);

Q_SIGNALS:
	void errorOccured(ErrorType type,
					  const QString &errorMessage,
					  const QVariant &errorData,
					  QPrivateSignal = {});

	void liveSyncEnabledChanged(bool liveSyncEnabled, QPrivateSignal = {});

private:
	friend class Setup;
	Q_DECLARE_PRIVATE(Engine)

	Q_PRIVATE_SLOT(d_func(), void _q_handleError(ErrorType, const QString &, const QVariant &))
	Q_PRIVATE_SLOT(d_func(), void _q_handleNetError(const QString &))
	Q_PRIVATE_SLOT(d_func(), void _q_handleLiveError(const QString &, const QString &, bool))
	Q_PRIVATE_SLOT(d_func(), void _q_signInSuccessful(const QString &, const QString &))
	Q_PRIVATE_SLOT(d_func(), void _q_accountDeleted(bool))
	Q_PRIVATE_SLOT(d_func(), void _q_triggerSync(bool))
	Q_PRIVATE_SLOT(d_func(), void _q_syncDone(const QString &))
	Q_PRIVATE_SLOT(d_func(), void _q_downloadedData(const QString &, const QList<CloudData> &))
	Q_PRIVATE_SLOT(d_func(), void _q_uploadedData(const ObjectKey &, const QDateTime &))
	Q_PRIVATE_SLOT(d_func(), void _q_triggerCloudSync(const QString &))
	Q_PRIVATE_SLOT(d_func(), void _q_transformDownloadDone(const LocalData &))
	Q_PRIVATE_SLOT(d_func(), void _q_removedUser())

	explicit Engine(QScopedPointer<SetupPrivate> &&setup, QObject *parent = nullptr);
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
