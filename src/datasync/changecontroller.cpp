#include "changecontroller_p.h"
#include "datastore.h"
#include "setup_p.h"
#include "exchangeengine_p.h"
#include "synchelper_p.h"
#include "changeemitter_p.h"

using namespace QtDataSync;

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

ChangeController::ChangeController(const Defaults &defaults, QObject *parent) :
	Controller("change", defaults, parent),
	_store(nullptr),
	_emitter(nullptr),
	_uploadingEnabled(false),
	_uploadLimit(10), //good default
	_activeUploads(),
	_changeEstimate(0)
{}

void ChangeController::initialize(const QVariantHash &params)
{
	_store = params.value(QStringLiteral("store")).value<LocalStore*>();
	Q_ASSERT_X(_store, Q_FUNC_INFO, "Missing parameter: store (LocalStore)");
	_emitter = params.value(QStringLiteral("emitter")).value<ChangeEmitter*>();
	Q_ASSERT_X(_emitter, Q_FUNC_INFO, "Missing parameter: emitter (ChangeEmitter)");

	connect(_emitter, &ChangeEmitter::uploadNeeded,
			this, &ChangeController::changeTriggered);
}

void ChangeController::setUploadingEnabled(bool uploading)
{
	_uploadingEnabled = uploading;
	logDebug() << "Change uploading enabled to" << uploading;
	if(uploading)
		uploadNext(true);
	else {
		endOp(); //stop timeouts
		emit uploadingChanged(false);
	}
}

void ChangeController::clearUploads()
{
	setUploadingEnabled(false);
	if(!_activeUploads.isEmpty())
		logDebug() << "Finished uploading changes";
	_activeUploads.clear();
	_changeEstimate = 0;
}

void ChangeController::updateUploadLimit(quint32 limit)
{
	logDebug() << "Updated update limit to:" << limit;
	_uploadLimit = static_cast<int>(limit);
}

void ChangeController::uploadDone(const QByteArray &key)
{
	if(!_activeUploads.contains(key)) {
		logWarning() << "Unknown key completed:" << key.toHex();
		return;
	}

	try {
		auto info = _activeUploads.take(key);
		_store->markUnchanged(info.key, info.version, info.isDelete);
		_changeEstimate--;
		emit progressIncrement();
		logDebug() << "Completed upload. Marked"
				   << info.key << "as unchanged ( Active uploads:"
				   << _activeUploads.size() << ")";

		if(_uploadingEnabled && _activeUploads.size() < _uploadLimit) //queued, so we may have the luck to complete a few more before uploading again
			QMetaObject::invokeMethod(this, "uploadNext", Qt::QueuedConnection,
									  Q_ARG(bool, false));
	} catch(Exception &e) {
		logCritical() << "Failed to complete upload with error:" << e.what();
		emit controllerError(tr("Failed to upload changes to server."));
	}
}

void ChangeController::deviceUploadDone(const QByteArray &key, const QUuid &deviceId)
{
	if(!_activeUploads.contains({key, deviceId})) {
		logWarning() << "Unknown device key completed:" << key.toHex() << deviceId;
		return;
	}

	try {
		auto info = _activeUploads.take({key, deviceId});
		_store->removeDeviceChange(info.key, deviceId);
		_changeEstimate--;
		emit progressIncrement();
		logDebug() << "Completed device upload. Marked"
				   << info.key << "for device" << deviceId << "as unchanged ( Active uploads:"
				   << _activeUploads.size() << ")";

		if(_uploadingEnabled && _activeUploads.size() < _uploadLimit) //queued, so we may have the luck to complete a few more before uploading again
			QMetaObject::invokeMethod(this, "uploadNext", Qt::QueuedConnection,
									  Q_ARG(bool, false));
	} catch(Exception &e) {
		logCritical() << "Failed to complete upload with error:" << e.what();
		emit controllerError(tr("Failed to upload changes to server."));
	}
}

void ChangeController::changeTriggered()
{
	if(_uploadingEnabled)
		uploadNext(_activeUploads.isEmpty());
}

void ChangeController::uploadNext(bool emitStarted)
{
	//uploads already exists: emit started no matter whether any are actually started from this call
	if(emitStarted && !_activeUploads.isEmpty()) {
		emitStarted = false;
		logDebug() << "Beginning uploading changes";
		emit uploadingChanged(true);
	}

	if(_activeUploads.size() >= _uploadLimit)
		return;

	try {
		//update change estimate, if neccessary
		auto emitProgress = false;
		if(_changeEstimate == 0) {
			_changeEstimate = _store->changeCount();
			if(_changeEstimate > 0) {
				if(emitStarted)
					emitProgress = true;
				else
					emit progressAdded(_changeEstimate);
			}
		}

		_store->loadChanges(_uploadLimit, [this, emitProgress, &emitStarted](ObjectKey objKey, quint64 version, QString file, QUuid deviceId) {
			CachedObjectKey key(objKey, deviceId);

			//skip stuff already beeing uploaded (could still have changed, but to prevent errors)
			auto skip = false;
			for(auto mKey : _activeUploads.keys()) {
				if(key == mKey) {
					skip = true;
					break;
				}
			}
			if(skip)
				return true;

			//signale that uploading has started
			if(emitStarted) {
				emitStarted = false;
				logDebug() << "Beginning uploading changes";
				emit uploadingChanged(true);
				if(emitProgress)
					emit progressAdded(_changeEstimate);
			}

			auto keyHash = key.hashed();
			auto isDelete = file.isNull();
			_activeUploads.insert(key, {key, version, isDelete});
			beginOp(); //start the default timeout
			if(isDelete) {//deleted
				if(deviceId.isNull()) {
					emit uploadChange(keyHash, SyncHelper::combine(key, version));
					logDebug() << "Started upload of deleted" << key
							   << "( Active uploads:" << _activeUploads.size() << ")";
				} else {
					emit uploadDeviceChange(keyHash, deviceId, SyncHelper::combine(key, version));
					logDebug() << "Started device upload of deleted"
							   << key << "for device" << deviceId
							   << "( Active uploads:" << _activeUploads.size() << ")";
				}
			} else { //changed
				try {
					auto json = _store->readJson(key, file);
					if(deviceId.isNull()) {
						emit uploadChange(keyHash, SyncHelper::combine(key, version, json));
						logDebug() << "Started upload of changed" << key
								   << "( Active uploads:" << _activeUploads.size() << ")";
					} else {
						emit uploadDeviceChange(keyHash, deviceId, SyncHelper::combine(key, version, json));
						logDebug() << "Started device upload of changed"
								   << key << "for device" << deviceId
								   << "( Active uploads:" << _activeUploads.size() << ")";
					}
				} catch (Exception &e) {
					logWarning() << "Failed to read json for upload. Assuming unchanged. Error:" << e.what();
					QMetaObject::invokeMethod(this, "uploadDone", Qt::QueuedConnection,
											  Q_ARG(QByteArray, keyHash));
				}
			}

			return _activeUploads.size() < _uploadLimit; //only continue as long as there is free space
		});

		if(_activeUploads.isEmpty()) {
			endOp(); //stop any timeouts
			logDebug() << "Finished uploading changes";
			emit uploadingChanged(false);
		}
	} catch(Exception &e) {
		logCritical() << "Error when trying to upload change:" << e.what();
		emit controllerError(tr("Failed to upload changes to server."));
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



ChangeController::CachedObjectKey::CachedObjectKey() :
	ObjectKey(),
	_hash()
{}

ChangeController::CachedObjectKey::CachedObjectKey(const ObjectKey &other, const QUuid &deviceId) :
	ObjectKey(other),
	optionalDevice(deviceId),
	_hash()
{}

ChangeController::CachedObjectKey::CachedObjectKey(const QByteArray &hash, const QUuid &deviceId) :
	ObjectKey(),
	optionalDevice(deviceId),
	_hash(hash)
{}

QByteArray ChangeController::CachedObjectKey::hashed() const
{
	if(_hash.isEmpty())
		_hash = ObjectKey::hashed();
	return _hash;
}

bool ChangeController::CachedObjectKey::operator==(const CachedObjectKey &other) const
{
	if(!_hash.isEmpty() && !other._hash.isEmpty())
		return _hash == other._hash && optionalDevice == other.optionalDevice;
	else
		return (static_cast<ObjectKey>(*this) == static_cast<ObjectKey>(other)) && optionalDevice == other.optionalDevice;
}

bool ChangeController::CachedObjectKey::operator!=(const CachedObjectKey &other) const
{
	if(!_hash.isEmpty() && !other._hash.isEmpty())
		return _hash != other._hash || optionalDevice != other.optionalDevice;
	else
		return (static_cast<ObjectKey>(*this) != static_cast<ObjectKey>(other)) || optionalDevice != other.optionalDevice;
}

uint QtDataSync::qHash(const ChangeController::CachedObjectKey &key, uint seed)
{
	return qHash(key.hashed(), seed) ^ qHash(key.optionalDevice, seed);
}
