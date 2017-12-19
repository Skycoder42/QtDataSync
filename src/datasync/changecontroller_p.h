#ifndef CHANGECONTROLLER_P_H
#define CHANGECONTROLLER_P_H

#include <QtCore/QObject>

#include <QtSql/QSqlDatabase>

#include "qtdatasync_global.h"
#include "objectkey.h"
#include "controller_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ChangeController : public Controller
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

	explicit ChangeController(const Defaults &defaults, QObject *parent = nullptr);

	void initialize() final;

	static bool createTables(Defaults defaults, QSqlDatabase database, bool canWrite = false);

	static void triggerDataChange(Defaults defaults, QSqlDatabase database, const ChangeInfo &changeInfo, const QWriteLocker &);
	static void triggerDataClear(Defaults defaults, QSqlDatabase database, const QByteArray &typeName, const QWriteLocker &);

	QList<ChangeInfo> loadChanges();
	void clearChanged(const ObjectKey &key, quint64 version);

	QByteArrayList loadClears();
	void clearCleared(const QByteArray &typeName);

public Q_SLOTS:
	void setUploadingEnabled(bool uploading);

Q_SIGNALS:
	void uploadingChanged(bool uploading);

private Q_SLOTS:
	void changeTriggered();

private:
	//app global lock, since the init cache is shared by all setups
	static QReadWriteLock _initLock;
	static QHash<QString, bool> _initialized;

	DatabaseRef _database;
	bool _uploadingEnabled;

	void exec(QSqlQuery &query, const ObjectKey &key = ObjectKey{"any"}) const;
	static void exec(Defaults defaults, QSqlQuery &query, const ObjectKey &key = ObjectKey{"any"});
	bool createTables();
};

}

Q_DECLARE_METATYPE(QtDataSync::ChangeController::ChangeInfo)

#endif // CHANGECONTROLLER_P_H
