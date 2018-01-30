#ifndef QTDATASYNC_CHANGECONTROLLER_P_H
#define QTDATASYNC_CHANGECONTROLLER_P_H

#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QUuid>

#include "qtdatasync_global.h"
#include "objectkey.h"
#include "controller_p.h"
#include "localstore_p.h"

namespace QtDataSync {

class ChangeEmitter;

class Q_DATASYNC_EXPORT ChangeController : public Controller
{
	Q_OBJECT

public:
	struct Q_DATASYNC_EXPORT ChangeInfo {
		ObjectKey key;
		quint64 version;
		QByteArray checksum;

		ChangeInfo();
		ChangeInfo(const ObjectKey &key, quint64 version, const QByteArray &checksum = {});
	};

	//not exported, because of "cheating" api. Declared public because of qHash
	class CachedObjectKey : public ObjectKey {
	public:
		CachedObjectKey();
		CachedObjectKey(const ObjectKey &other, const QUuid &deviceId = {});
		CachedObjectKey(const QByteArray &hash, const QUuid &deviceId = {});

		QByteArray hashed() const;

		QUuid optionalDevice;

		bool operator==(const CachedObjectKey &other) const;
		bool operator!=(const CachedObjectKey &other) const;

	private:
		mutable QByteArray _hash;
	};

	explicit ChangeController(const Defaults &defaults, QObject *parent = nullptr);

	void initialize(const QVariantHash &params) final;

public Q_SLOTS:
	void setUploadingEnabled(bool uploading);
	void clearUploads();
	void updateUploadLimit(quint32 limit);

	void uploadDone(const QByteArray &key);
	void deviceUploadDone(const QByteArray &key, const QUuid &deviceId);

Q_SIGNALS:
	void uploadingChanged(bool uploading);
	void uploadChange(const QByteArray &key, const QByteArray &changeData);
	void uploadDeviceChange(const QByteArray &key, const QUuid &deviceId, const QByteArray &changeData);

private Q_SLOTS:
	void changeTriggered();
	void uploadNext(bool emitStarted = false);

private:
	//unexported private member
	struct UploadInfo {
		ObjectKey key;
		quint64 version;
		bool isDelete;
	};

	LocalStore *_store;
	ChangeEmitter *_emitter;
	bool _uploadingEnabled;
	int _uploadLimit;
	QHash<CachedObjectKey, UploadInfo> _activeUploads;
	quint32 _changeEstimate;
};

//not exported, just like the class
uint qHash(const ChangeController::CachedObjectKey &key, uint seed);

}

Q_DECLARE_METATYPE(QtDataSync::ChangeController::ChangeInfo)
Q_DECLARE_TYPEINFO(QtDataSync::ChangeController::ChangeInfo, Q_MOVABLE_TYPE);

#endif // QTDATASYNC_CHANGECONTROLLER_P_H
