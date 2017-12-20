#include "changecontroller_p.h"
#include "datastore.h"
#include "setup_p.h"
#include "exchangeengine_p.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

using namespace QtDataSync;

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

ChangeController::ChangeController(const Defaults &defaults, QObject *parent) :
	Controller("change", defaults, parent),
	_database(),
	_uploadingEnabled(false)
{}

void ChangeController::initialize()
{
	_database = defaults().aquireDatabase(this);
	_database.createGlobalScheme(defaults());
}

void ChangeController::triggerDataChange(Defaults defaults, const QWriteLocker &)
{
	auto instance = SetupPrivate::engine(defaults.setupName())->changeController();
	QMetaObject::invokeMethod(instance, "changeTriggered", Qt::QueuedConnection);
}

void ChangeController::setUploadingEnabled(bool uploading)
{
	_uploadingEnabled = uploading;
	//TODO check if changes ar available
	emit uploadingChanged(false);
}

void ChangeController::changeTriggered()
{
	if(_uploadingEnabled) {
		emit uploadingChanged(true);
		//TODO implement
	}
}

void ChangeController::exec(QSqlQuery &query, const ObjectKey &key) const
{
	exec(defaults(), query, key);
}

void ChangeController::exec(Defaults defaults, QSqlQuery &query, const ObjectKey &key)
{
	if(!query.exec()) {
		throw LocalStoreException(defaults,
								  key,
								  query.executedQuery().simplified(),
								  query.lastError().text());
	}
}



ChangeController::ChangeInfo::ChangeInfo() :
	key(),
	version(0),
	checksum()
{}

ChangeController::ChangeInfo::ChangeInfo(const ObjectKey &key, quint64 version, const QByteArray &checksum) :
	key(key),
	version(version),
	checksum(checksum)
{}
