#include "changecontroller_p.h"
#include "datastore.h"
#include "setup_p.h"
#include "exchangeengine_p.h"
#include "synchelper_p.h"

using namespace QtDataSync;

#define QTDATASYNC_LOG QTDATASYNC_LOG_CONTROLLER

const int ChangeController::UploadLimit = 10;

ChangeController::ChangeController(const Defaults &defaults, QObject *parent) :
	Controller("change", defaults, parent),
	_store(nullptr),
	_uploadingEnabled(false),
	_activeUploads()
{}

void ChangeController::initialize(const QVariantHash &params)
{
	_store = params.value(QStringLiteral("store")).value<LocalStore*>();
	Q_ASSERT_X(_store, Q_FUNC_INFO, "Missing parameter: store (LocalStore)");
}

void ChangeController::triggerDataChange(Defaults defaults, const QWriteLocker &)
{
	auto instance = SetupPrivate::engine(defaults.setupName())->changeController();
	QMetaObject::invokeMethod(instance, "changeTriggered", Qt::QueuedConnection);
}

void ChangeController::setUploadingEnabled(bool uploading)
{
	_uploadingEnabled = uploading;
	if(uploading && canUpload()) {
		emit uploadingChanged(true);
		uploadNext();
	} else
		emit uploadingChanged(false);
}

void ChangeController::clearUploads()
{
	setUploadingEnabled(false);
	_activeUploads.clear();
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
		logDebug() << "Completed upload. Marked" << info.key << "as unchanged ( Active uploads:" << _activeUploads.size() << ")";

		if(_activeUploads.size() < UploadLimit) //queued, so we may have the luck to complete a few more before uploading again
			QMetaObject::invokeMethod(this, "uploadNext", Qt::QueuedConnection);
	} catch(Exception &e) {
		logCritical() << "Failed to complete upload with error:" << e.what();
		emit controllerError(tr("Failed to upload changes to server."));
	}
}

void ChangeController::changeTriggered()
{
	if(_uploadingEnabled && canUpload()) {
		emit uploadingChanged(true);
		uploadNext();
	}
}

void ChangeController::uploadNext()
{
	if(_activeUploads.size() >= UploadLimit)
		return;

	try {
		_store->loadChanges(UploadLimit, [this](ObjectKey objKey, quint64 version, QString file) {
			CachedObjectKey key(objKey);

			auto skip = false;
			foreach(auto mKey, _activeUploads.keys()) {
				if(key == mKey) {
					skip = true;
					break;
				}
			}
			if(skip)
				return true;

			auto keyHash = key.hashed();
			auto isDelete = file.isNull();
			_activeUploads.insert(key, {key, version, isDelete});
			if(isDelete) {//deleted
				emit uploadChange(keyHash, SyncHelper::combine(key, version));
				logDebug() << "Started upload of deleted" << key << "( Active uploads:" << _activeUploads.size() << ")";
			} else { //changed
				try {
					auto json = _store->readJson(key, file);
					emit uploadChange(keyHash, SyncHelper::combine(key, version, json));
					logDebug() << "Started upload of changed" << key << "( Active uploads:" << _activeUploads.size() << ")";
				} catch (Exception &e) {
					logWarning() << "Failed to read json for upload. Assuming unchanged. Error:" << e.what();
					QMetaObject::invokeMethod(this, "uploadDone", Qt::QueuedConnection,
											  Q_ARG(QByteArray, keyHash));
				}
			}

			return _activeUploads.size() < UploadLimit; //only continue as long as there is free space
		});
	} catch(Exception &e) {
		logCritical() << "Error when trying to upload change:" << e.what();
		emit controllerError(tr("Failed to upload changes to server."));
	}
}

bool ChangeController::canUpload()
{
	if(!_activeUploads.isEmpty())
		return true;
	else {
		try {
			return _store->hasChanges();
		} catch(Exception &e) {
			logCritical() << "Failed to check changes with error:" << e.what();
			emit controllerError(tr("Failed to upload changes to server."));
			return false;
		}
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

ChangeController::CachedObjectKey::CachedObjectKey(const ObjectKey &other) :
	ObjectKey(other),
	_hash()
{}

ChangeController::CachedObjectKey::CachedObjectKey(const QByteArray &hash) :
	ObjectKey(),
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
		return _hash == other._hash;
	else
		return ((ObjectKey)(*this) == (ObjectKey)other);
}

bool ChangeController::CachedObjectKey::operator!=(const CachedObjectKey &other) const
{
	if(!_hash.isEmpty() && !other._hash.isEmpty())
		return _hash != other._hash;
	else
		return ((ObjectKey)(*this) != (ObjectKey)other);
}

uint QtDataSync::qHash(const ChangeController::CachedObjectKey &key, uint seed)
{
	return qHash(key.hashed(), seed);
}
