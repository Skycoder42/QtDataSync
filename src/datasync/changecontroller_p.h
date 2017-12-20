#ifndef CHANGECONTROLLER_P_H
#define CHANGECONTROLLER_P_H

#include <QtCore/QObject>
#include <QtCore/QMutex>

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

	static void triggerDataChange(Defaults defaults, const QWriteLocker &);

public Q_SLOTS:
	void setUploadingEnabled(bool uploading);

Q_SIGNALS:
	void uploadingChanged(bool uploading);

private Q_SLOTS:
	void changeTriggered();

private:
	DatabaseRef _database;
	bool _uploadingEnabled;

	void exec(QSqlQuery &query, const ObjectKey &key = ObjectKey{"any"}) const;
	static void exec(Defaults defaults, QSqlQuery &query, const ObjectKey &key = ObjectKey{"any"});
	bool createTables();
};

}

Q_DECLARE_METATYPE(QtDataSync::ChangeController::ChangeInfo)

#endif // CHANGECONTROLLER_P_H
