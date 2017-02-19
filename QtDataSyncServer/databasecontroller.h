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
	QJsonValue loadChanges(const QUuid &userId, const QUuid &deviceId);
	QJsonValue load(const QUuid &userId, const QString &type, const QString &key);
	bool save(const QUuid &userId, const QUuid &deviceId, const QString &type, const QString &key, const QJsonObject &object);
	bool remove(const QUuid &userId, const QUuid &deviceId, const QString &type, const QString &key);
	bool markUnchanged(const QUuid &userId, const QUuid &deviceId, const QString &type, const QString &key);

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
	bool markStateUnchanged(QSqlDatabase &database, const QUuid &userId, const QUuid &deviceId, quint64 index);
	bool updateDeviceStates(QSqlDatabase &database, const QUuid &userId, const QUuid &deviceId, quint64 index);
	void tryDeleteData(QSqlDatabase &database, quint64 index);

	QString jsonToString(const QJsonObject &object) const;
	QJsonObject stringToJson(const QString &data) const;
};

#endif // DATABASECONTROLLER_H
