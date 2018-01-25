#include "synccontroller_p.h"
#include "synchelper_p.h"
#include "conflictresolver.h"

using namespace QtDataSync;
using std::tie;

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

SyncController::SyncController(const Defaults &defaults, QObject *parent) :
	Controller("sync", defaults, parent),
	_store(nullptr),
	_enabled(false)
{}

void SyncController::initialize(const QVariantHash &params)
{
	_store = params.value(QStringLiteral("store")).value<LocalStore*>();
	Q_ASSERT_X(_store, Q_FUNC_INFO, "Missing parameter: store (LocalStore)");
}

void SyncController::setSyncEnabled(bool enabled)
{
	_enabled = enabled;
}

void SyncController::syncChange(quint64 key, const QByteArray &changeData)
{
	if(!_enabled)
		return;

	try {
		bool remoteDeleted;
		ObjectKey objKey;
		quint64 remoteVersion;
		QJsonObject remoteData;
		tie(remoteDeleted, objKey, remoteVersion, remoteData) = SyncHelper::extract(changeData);

		auto scope = _store->startSync(objKey);
		LocalStore::ChangeType localState;
		quint64 localVersion;
		QString localFileName;
		QByteArray localChecksum;
		tie(localState, localVersion, localFileName, localChecksum) = _store->loadChangeInfo(scope);

		const char *syncActionStr = "invalid";
		const char *syncActionRes = "invalid";

		switch (localState) {
		case LocalStore::Exists:
			if(remoteDeleted) { // exists<->deleted
				syncActionStr = "exists<->deleted";
				if(localVersion < remoteVersion) {
					auto persist = defaults().property(Defaults::PersistDeleted).toBool();
					_store->storeDeleted(scope, remoteVersion, !persist, localState); //store the delete either unchanged or changed, see exchange.txt
					syncActionRes = "remote";
				} else if(localVersion == remoteVersion) {
					switch (static_cast<Setup::SyncPolicy>(defaults().property(Defaults::ConflictPolicy).toInt())) {
					case Setup::PreferChanged:
						_store->updateVersion(scope, localVersion, localVersion + 1ull, true); //keep as "v1 + 1"
						syncActionRes = "local";
						break;
					case Setup::PreferDeleted:
						_store->storeDeleted(scope, remoteVersion + 1ull, true, localState); //store as "v2 + 1"
						syncActionRes = "remote";
						break;
					default:
						Q_UNREACHABLE();
						break;
					}
				} else //(localVersion > remoteVersion): do nothing
					syncActionRes = "local";
			} else { // exists<->changed
				syncActionStr = "exists<->changed";
				if(localVersion < remoteVersion) {
					_store->storeChanged(scope, remoteVersion, localFileName, remoteData, false, localState); //simply update the local data
					syncActionRes = "remote";
				} else if(localVersion == remoteVersion) {
					auto remoteChecksum = SyncHelper::jsonHash(remoteData);
					if(localChecksum != remoteChecksum) { //conflict!
						QJsonObject resolvedData;
						auto resolver = defaults().conflictResolver();
						if(resolver) {
							auto localData = _store->readJson(objKey, localFileName);
							resolvedData = resolver->resolveConflict(QMetaType::type(objKey.typeName.constData()), localData, remoteData);
						}
						//deterministic alg the chooses 1 dataset no matter which one is local
						if(!resolvedData.isEmpty()) {
							_store->storeChanged(scope, localVersion + 1ull, localFileName, resolvedData, true, localState); //store as "v2 + 1"
							syncActionRes = "merged";
						} else if(localChecksum > remoteChecksum) {
							_store->updateVersion(scope, localVersion, localVersion + 1ull, true); //keep as "v1 + 1"
							syncActionRes = "local";
						} else {
							_store->storeChanged(scope, remoteVersion + 1ull, localFileName, remoteData, true, localState); //store as "v2 + 1"
							syncActionRes = "remote";
						}
					} else {//(localChecksum == remoteChecksum): mark unchanged, if it was changed, because same data does not need another upload
						_store->markUnchanged(scope, localVersion, false);
						syncActionRes = "identical";
					}
				} else //(localVersion > remoteVersion): do nothing
					syncActionRes = "local";
			}
			break;
		case LocalStore::ExistsDeleted:
			if(remoteDeleted) { // cachedDelete<->deleted
				syncActionStr = "cachedDelete<->deleted";
				syncActionRes = "identical";
				if(localVersion <= remoteVersion) {
					if(defaults().property(Defaults::PersistDeleted).toBool()) //when persisting, store the delete
						_store->updateVersion(scope, localVersion, remoteVersion, false);
					else //if not, simply delete the cached delete as it is not needed anymore
						_store->markUnchanged(scope, localVersion, true); //pass local version to make shure it's accepted
				} //else: do nothing
			} else { // cachedDelete<->changed
				syncActionStr = "cachedDelete<->changed";
				if(localVersion < remoteVersion) {
					_store->storeChanged(scope, remoteVersion, localFileName, remoteData, false, localState); //simply update the local data
					syncActionRes = "remote";
				} else if(localVersion == remoteVersion) {
					switch (static_cast<Setup::SyncPolicy>(defaults().property(Defaults::ConflictPolicy).toInt())) {
					case Setup::PreferChanged:
						_store->storeChanged(scope, remoteVersion + 1ull, localFileName, remoteData, true, localState); //store as "v2 + 1"
						syncActionRes = "remote";
						break;
					case Setup::PreferDeleted:
						_store->updateVersion(scope, localVersion, localVersion + 1ull, true); //keep as "v1 + 1"
						syncActionRes = "local";
						break;
					default:
						Q_UNREACHABLE();
						break;
					}
				} else //(localVersion > remoteVersion): do nothing
					syncActionRes = "local";
			}
			break;
		case LocalStore::NoExists:
			if(remoteDeleted) { // noexists<->deleted
				syncActionStr = "noexists<->deleted";
				syncActionRes = "identical";
				if(defaults().property(Defaults::PersistDeleted).toBool()) //when persisting, store the delete
					_store->storeDeleted(scope, remoteVersion, false, localState);
				//else: do nothing
			} else { // noexists<->changed
				syncActionStr = "noexists<->changed";
				syncActionRes = "remote";
				//no additional info, simply take it (See exchange.txt)
				_store->storeChanged(scope, remoteVersion, localFileName, remoteData, false, localState);
			}
			break;
		default:
			Q_UNREACHABLE();
			break;
		}

		logDebug().nospace() << "Synced " << objKey
							 << " with action(" << syncActionStr << "), result is data of: "
							 << syncActionRes;

		_store->commitSync(scope);
		emit syncDone(key);
	} catch (QException &e) {
		logCritical() << "Failed to synchronize data:" << e.what();
		emit controllerError(tr("Data downloaded from server is invalid."));
	}
}
