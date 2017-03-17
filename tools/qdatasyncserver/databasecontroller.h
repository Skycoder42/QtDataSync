#ifndef DATABASECONTROLLER_H
#define DATABASECONTROLLER_H

#include <QObject>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QThreadPool>
#include <QThreadStorage>
#include <QUuid>

class DatabaseController : public QObject
{
	Q_OBJECT

public:
	explicit DatabaseController(QObject *parent = nullptr);

	QUuid createIdentity(const QUuid &deviceId, bool &resync);
	bool identify(const QUuid &identity, const QUuid &deviceId, bool &resync);
	QJsonValue loadChanges(const QUuid &userId, const QUuid &deviceId, bool resync);
	QJsonValue load(const QUuid &userId, const QString &type, const QString &key);
	bool save(const QUuid &userId, const QUuid &deviceId, const QString &type, const QString &key, const QJsonObject &object);
	bool remove(const QUuid &userId, const QUuid &deviceId, const QString &type, const QString &key);
	bool markUnchanged(const QUuid &userId, const QUuid &deviceId, const QString &type, const QString &key);
	void deleteOldDevice(const QUuid &userId, const QUuid &deviceId);

	void cleanupDevices(quint64 offlineSinceDays);
	void cleanupUsers();
	void cleanupData();

signals:
	void notifyChanged(const QUuid &userId,
					   const QUuid &excludedDeviceId,
					   const QString &type,
					   const QString &key,
					   bool changed);

	void databaseInitDone(bool success);
	void cleanupOperationDone(int rowsAffected, const QString &error = {});

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
