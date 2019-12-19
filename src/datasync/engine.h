#ifndef QTDATASYNC_ENGINE_H
#define QTDATASYNC_ENGINE_H

#include "QtDataSync/qtdatasync_global.h"

#include <QtCore/qobject.h>

#include <QtSql/qsqldatabase.h>

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

public:
	IAuthenticator *authenticator() const;
	ICloudTransformer* transformer() const;

	Q_INVOKABLE bool syncDb(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
							const QStringList &tables = {});
	Q_INVOKABLE bool syncDb(QSqlDatabase database,
							const QStringList &tables = {});

	Q_INVOKABLE bool syncTable(const QString &table,
							   const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
							   const QStringList &fields = {},
							   const QString &primaryKeyType = {});
	Q_INVOKABLE bool syncTable(const QString &table,
							   QSqlDatabase database,
							   const QStringList &fields = {},
							   const QString &primaryKeyType = {});

public Q_SLOTS:
	void start();

	void removeDbSync(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
					  const QStringList &tables = {});
	void removeDbSync(QSqlDatabase database,
					  const QStringList &tables = {});
	void removeTableSync(const QString &table,
						 const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void removeTableSync(const QString &table,
						 QSqlDatabase database);

	void unsyncDb(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
				  const QStringList &tables = {});
	void unsyncDb(QSqlDatabase database,
				  const QStringList &tables = {});
	void unsyncTable(const QString &table,
					 const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void unsyncTable(const QString &table,
					 QSqlDatabase database);

private:
	friend class Setup;
	Q_DECLARE_PRIVATE(Engine)

	explicit Engine(QScopedPointer<SetupPrivate> &&setup, QObject *parent = nullptr);
};

}

#endif // QTDATASYNC_ENGINE_H
