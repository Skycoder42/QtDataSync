#ifndef QTDATASYNC_DATABASEWATCHER_H
#define QTDATASYNC_DATABASEWATCHER_H

#include "qtdatasync_global.h"

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlField>

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

	class Q_DATASYNC_EXPORT ITypeConverter
	{
		Q_DISABLE_COPY(ITypeConverter)
	public:
		ITypeConverter();
		virtual ~ITypeConverter();

		virtual QString sqlTypeName(const QSqlField &field) const = 0;
	};

	explicit DatabaseWatcher(QSqlDatabase &&db,
							 ITypeConverter *typeConverter,
							 QObject *parent = nullptr);

	void addAllTables(QSql::TableType type);
	void addTable(const QString &name, QString primaryType = {});

Q_SIGNALS:
	void databaseError(const QString &dbConName, const QString &error, QPrivateSignal);

private:
	QSqlDatabase _db;
	QScopedPointer<ITypeConverter> _typeConverter;
};

class Q_DATASYNC_EXPORT SqliteTypeConverter : public DatabaseWatcher::ITypeConverter
{
public:
	QString sqlTypeName(const QSqlField &field) const override;
};

Q_DECLARE_LOGGING_CATEGORY(logDbWatcher)

}

#endif // QTDATASYNC_DATABASEWATCHER_H
