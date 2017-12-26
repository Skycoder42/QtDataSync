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
		bool remoteDeleted;
		ObjectKey objKey;
		quint64 remoteVersion;
		QJsonObject remoteData;
		std::tie(remoteDeleted, objKey, remoteVersion, remoteData) = SyncHelper::extract(changeData);

		auto scope = _store->startSync(objKey);
		LocalStore::ChangeType localState;
		quint64 localVersion;
		QByteArray localChecksum;
		std::tie(localState, localVersion, localChecksum) = _store->loadChangeInfo(scope);

		switch (localState) {
		case LocalStore::Exists:
			if(remoteDeleted) { // exists<->deleted

			} else { // exists<->changed

			}
			break;
		case LocalStore::ExistsDeleted:
			if(remoteDeleted) { // cachedDelete<->deleted

			} else { // cachedDelete<->changed

			}
			break;
		case LocalStore::NoExists:
			if(remoteDeleted) { // noexists<->deleted
				if(defaults().property(Defaults::PersistDeleted).toBool())
					_store->storeDeleted(scope, remoteVersion, false, localState);
				//else: do nothing
				logDebug() << "Synced as action(noexists<->deleted)";
			} else { // noexists<->changed

			}
			break;
		default:
			Q_UNREACHABLE();
			break;
		}

		//TODO implement

		_store->commitSync(scope);
		emit syncDone(key);
	} catch (QException &e) {
		logCritical() << "Failed to synchronize data:" << e.what();
		emit controllerError(tr("Data downloaded from server is invalid."));
	}
}
