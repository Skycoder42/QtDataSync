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

	QUuid createIdentity(const QUuid &deviceId);
	bool identify(const QUuid &identity, const QUuid &deviceId);
	bool save(const QUuid &userId, const QUuid &deviceId, const QString &type, const QString &key, const QJsonObject &object);

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

	QString jsonToString(const QJsonObject &object) const;
};

#endif // DATABASECONTROLLER_H
