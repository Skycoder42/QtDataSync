#ifndef DATABASECONTROLLER_H
#define DATABASECONTROLLER_H

#include <QObject>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QThreadPool>
#include <QThreadStorage>
#include <functional>

class DatabaseController : public QObject
{
	Q_OBJECT

public:
	explicit DatabaseController(QObject *parent = nullptr);
	~DatabaseController();

	void createIdentity(QObject *object, const QByteArray &method);//void(QUuid)
	void identify(const QUuid &identity, QObject *object, const QByteArray &method);//void(bool)

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
	QThreadPool *pool;
	QThreadStorage<DatabaseWrapper> threadStore;
};

#endif // DATABASECONTROLLER_H
