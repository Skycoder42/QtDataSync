#include "synccontroller_p.h"
#include "synchelper_p.h"

using namespace QtDataSync;

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

SyncController::SyncController(const Defaults &defaults, QObject *parent) :
	Controller("sync", defaults, parent),
	_store(nullptr)
{}

void SyncController::initialize(const QVariantHash &params)
{
	_store = params.value(QStringLiteral("store")).value<LocalStore*>();
	Q_ASSERT_X(_store, Q_FUNC_INFO, "Missing parameter: store (LocalStore)");
}

void SyncController::syncChange(quint64 key, const QByteArray &changeData)
{
	try {
		bool remoteState;
		ObjectKey key;
		quint64 remoteVersion;
		QJsonObject remoteData;
		std::tie(remoteState, key, remoteVersion, remoteData) = SyncHelper::extract(changeData);

		auto scope = _store->startSync(key);
		LocalStore::ChangeType localState;
		quint64 localVersion;
		std::tie(localState, localVersion) = _store->loadChangeInfo(scope);

		//TODO implement

		_store->commitSync(scope);
		logDebug() << "IT WORKED!!!";
	} catch (QException &e) {
		logCritical() << "Failed to synchronize data:" << e.what();
		emit controllerError(tr("Data downloaded from server is invalid."));
	}
}
