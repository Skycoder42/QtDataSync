#ifndef QTDATASYNC_ENGINE_H
#define QTDATASYNC_ENGINE_H

#include "QtDataSync/qtdatasync_global.h"

#include <QtCore/qobject.h>

#include <QtSql/qsqldatabase.h>

namespace QtDataSync {

class IAuthenticator;

class SetupPrivate;

class EnginePrivate;
class Q_DATASYNC_EXPORT Engine : public QObject
{
	Q_OBJECT

	Q_PROPERTY(IAuthenticator* authenticator READ authenticator CONSTANT)

public:
	IAuthenticator *authenticator() const;

	Q_INVOKABLE bool registerForSync(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
									 const QString &table = {},
									 const QStringList &fields = {});
	Q_INVOKABLE bool registerForSync(QSqlDatabase database,
									 const QString &table = {},
									 const QStringList &fields = {});
	Q_INVOKABLE bool removeFromSync(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection),
									const QString &table = {});

public Q_SLOTS:
	void start();

private:
	friend class Setup;
	Q_DECLARE_PRIVATE(Engine)

	explicit Engine(QScopedPointer<SetupPrivate> &&setup, QObject *parent = nullptr);
};

}

#endif // QTDATASYNC_ENGINE_H
