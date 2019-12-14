#ifndef QTDATASYNC_LOCALSTORE_H
#define QTDATASYNC_LOCALSTORE_H

#include "qtdatasync_global.h"
#include "engine.h"
#include "exception.h"

#include <optional>

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QThreadStorage>
#include <QtCore/QPointer>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

namespace QtDataSync {

class Q_DATASYNC_EXPORT LocalDatabase : public QObject
{
public:
	LocalDatabase(Engine *engine, QObject *parent);
	~LocalDatabase();

	operator QSqlDatabase() const;
	QSqlDatabase *operator->() const;

	bool eventFilter(QObject *watched, QEvent *event) final;

private:
	using DatabaseInfo = std::pair<QUuid, quint64>;
	static QThreadStorage<DatabaseInfo> _databaseIds;

	QPointer<Engine> _engine;
	mutable std::optional<QSqlDatabase> _database;

	void acquireDb() const;
	void releaseDb() const;
};

class Q_DATASYNC_EXPORT LocalStore : public QObject
{
	Q_OBJECT

public:
	explicit LocalStore(Engine *engine, QObject *parent = nullptr);

	quint64 count(const QByteArray &typeName) const;
	QStringList keys(const QByteArray &typeName) const;

private:
	static const QString IndexTable;

	LocalDatabase _database;

	void createTables();
};

class Q_DATASYNC_EXPORT LocalStoreException : public Exception
{
public:
	LocalStoreException(const QSqlError &error);
	LocalStoreException(const QSqlDatabase &database);
	LocalStoreException(const QSqlQuery &query);

	QString qWhat() const override;
	void raise() const override;
	ExceptionBase *clone() const override;

protected:
	QString _error;
};

}

#endif // QTDATASYNC_LOCALSTORE_H
