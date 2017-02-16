#ifndef DATABASECONTROLLER_H
#define DATABASECONTROLLER_H

#include <QObject>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QThreadPool>
#include <QThreadStorage>

class DatabaseController : public QObject
{
	Q_OBJECT

public:
	explicit DatabaseController(QObject *parent = nullptr);

	QUuid createIdentity();
	bool identify(const QUuid &identity);

private:
	class DatabaseWrapper
	{
	public:
		DatabaseWrapper();
		~DatabaseWrapper();

		QSqlDatabase database() const;
	private:
		QString dbName;
	};

	bool multiThreaded;
	QThreadStorage<DatabaseWrapper> threadStore;

	void initDatabase();
};

#endif // DATABASECONTROLLER_H
