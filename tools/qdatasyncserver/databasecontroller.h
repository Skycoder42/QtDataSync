#ifndef DATABASECONTROLLER_H
#define DATABASECONTROLLER_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QThreadPool>
#include <QtCore/QThreadStorage>
#include <QtCore/QUuid>
#include <QtCore/QJsonObject>

#include <QtSql/QSqlDatabase>

class DatabaseController : public QObject
{
	Q_OBJECT

public:
	explicit DatabaseController(QObject *parent = nullptr);

	void cleanupDevices(quint64 offlineSinceDays);

signals:
	//TODO correct
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

	QString jsonToString(const QJsonObject &object) const;
	QJsonObject stringToJson(const QString &data) const;
};

#endif // DATABASECONTROLLER_H
