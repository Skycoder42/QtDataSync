#include "synccontroller_p.h"
#include "synchelper_p.h"
#include "conflictresolver.h"

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
		QString localFileName;
		QByteArray localChecksum;
		std::tie(localState, localVersion, localFileName, localChecksum) = _store->loadChangeInfo(scope);

		switch (localState) {
		case LocalStore::Exists:
			if(remoteDeleted) { // exists<->deleted
				if(localVersion < remoteVersion) {
					auto persist = defaults().property(Defaults::PersistDeleted).toBool();
					_store->storeDeleted(scope, remoteVersion, !persist, localState); //store the delete either unchanged or changed, see exchange.txt
				} else if(localVersion == remoteVersion) {
					switch ((Setup::SyncPolicy)defaults().property(Defaults::ConflictPolicy).toInt()) {
					case Setup::PreferChanged:
						_store->updateVersion(scope, localVersion, localVersion + 1ull, true); //keep as "v1 + 1"
						break;
					case Setup::PreferDeleted:
						_store->storeDeleted(scope, remoteVersion + 1ull, true, localState); //store as "v2 + 1"
						break;
					default:
						Q_UNREACHABLE();
						break;
					}
				} //else (localVersion > remoteVersion): do nothing
				logDebug() << "Synced" << objKey << "with action(exists<->deleted)";
			} else { // exists<->changed
				if(localVersion < remoteVersion)
					_store->storeChanged(scope, remoteVersion, localFileName, remoteData, false, localState); //simply update the local data
				else if(localVersion == remoteVersion) {
					auto remoteChecksum = SyncHelper::jsonHash(remoteData);
					if(localChecksum != remoteChecksum) { //conflict!
						QJsonObject resolvedData;
						auto resolver = defaults().conflictResolver();
						if(resolver) {
							auto localData = _store->readJson(objKey, localFileName);
							resolvedData = resolver->resolveConflict(QMetaType::type(objKey.typeName.constData()), localData, remoteData);
						}
						//deterministic alg the chooses 1 dataset no matter which one is local
						if(!resolvedData.isEmpty())
							_store->storeChanged(scope, localVersion + 1ull, localFileName, resolvedData, true, localState); //store as "v2 + 1"
						else if(localChecksum > remoteChecksum)
							_store->updateVersion(scope, localVersion, localVersion + 1ull, true); //keep as "v1 + 1"
						else
							_store->storeChanged(scope, remoteVersion + 1ull, localFileName, remoteData, true, localState); //store as "v2 + 1"
					} else //(localChecksum == remoteChecksum): mark unchanged, if it was changed, because same data does not need another upload
						_store->markUnchanged(scope, localVersion, false);
				} //else (localVersion > remoteVersion): do nothing
				logDebug() << "Synced" << objKey << "with action(exists<->changed)";
			}
			break;
		case LocalStore::ExistsDeleted:
			if(remoteDeleted) { // cachedDelete<->deleted
				if(localVersion <= remoteVersion) {
					if(defaults().property(Defaults::PersistDeleted).toBool()) //when persisting, store the delete
						_store->updateVersion(scope, localVersion, remoteVersion, false);
					else //if not, simply delete the cached delete as it is not needed anymore
						_store->markUnchanged(scope, localVersion, true); //pass local version to make shure it's accepted
				} //else: do nothing
				logDebug() << "Synced" << objKey << "with action(cachedDelete<->deleted)";
			} else { // cachedDelete<->changed
				if(localVersion < remoteVersion)
					_store->storeChanged(scope, remoteVersion, localFileName, remoteData, false, localState); //simply update the local data
				else if(localVersion == remoteVersion) {
					switch ((Setup::SyncPolicy)defaults().property(Defaults::ConflictPolicy).toInt()) {
					case Setup::PreferChanged:
						_store->storeChanged(scope, remoteVersion + 1ull, localFileName, remoteData, true, localState); //store as "v2 + 1"
						break;
					case Setup::PreferDeleted:
						_store->updateVersion(scope, localVersion, localVersion + 1ull, true); //keep as "v1 + 1"
						break;
					default:
						Q_UNREACHABLE();
						break;
					}
				} //else (localVersion > remoteVersion): do nothing
				logDebug() << "Synced" << objKey << "with action(cachedDelete<->changed)";
			}
			break;
		case LocalStore::NoExists:
			if(remoteDeleted) { // noexists<->deleted
				if(defaults().property(Defaults::PersistDeleted).toBool()) //when persisting, store the delete
					_store->storeDeleted(scope, remoteVersion, false, localState);
				//else: do nothing
				logDebug() << "Synced" << objKey << "with action(noexists<->deleted)";
			} else { // noexists<->changed
				//no additional info, simply take it (See exchange.txt)
				_store->storeChanged(scope, remoteVersion, localFileName, remoteData, false, localState);
				logDebug() << "Synced" << objKey << "with action(noexists<->changed)";
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
