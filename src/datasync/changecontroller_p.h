#ifndef CHANGECONTROLLER_P_H
#define CHANGECONTROLLER_P_H

#include <QtCore/QObject>
#include <QtSql/QSqlDatabase>

#include "qtdatasync_global.h"
#include "objectkey.h"
#include "defaults.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ChangeController : public QObject
{
	Q_OBJECT

public:
	struct ChangeInfo {
		ObjectKey key;
		quint64 version;
		QByteArray checksum;

		ChangeInfo();
		ChangeInfo(const ObjectKey &key, quint64 version, const QByteArray &checksum = {});
	};

	explicit ChangeController(const QString &setupName, QObject *parent = nullptr);

	static bool createTables(Defaults defaults, QSqlDatabase database, bool canWrite = false);

	static void triggerDataChange(Defaults defaults, QSqlDatabase database, const ChangeInfo &changeInfo, const QWriteLocker &);
	static void triggerDataClear(Defaults defaults, QSqlDatabase database, const QByteArray &typeName, const QWriteLocker &);

	QList<ChangeInfo> loadChanges();
	void clearChanged(const ObjectKey &key, quint64 version);

	QByteArrayList loadClears();
	void clearCleared(const QByteArray &typeName);

Q_SIGNALS:
	void changeTriggered();

private:
	static bool initialized;

	Defaults defaults;
	DatabaseRef database;

	void exec(QSqlQuery &query, const ObjectKey &key = ObjectKey{"any"}) const;
	static void exec(Defaults defaults, QSqlQuery &query, const ObjectKey &key = ObjectKey{"any"});
	bool createTables();
};

}

Q_DECLARE_METATYPE(QtDataSync::ChangeController::ChangeInfo)

#endif // CHANGECONTROLLER_P_H
