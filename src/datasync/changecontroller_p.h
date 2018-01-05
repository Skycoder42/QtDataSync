#ifndef CHANGECONTROLLER_P_H
#define CHANGECONTROLLER_P_H

#include <QtCore/QObject>
#include <QtCore/QMutex>

#include "qtdatasync_global.h"
#include "objectkey.h"
#include "controller_p.h"
#include "localstore_p.h"

namespace QtDataSync {

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

	//INTERNAL USE ONLY
	class Q_DATASYNC_EXPORT CachedObjectKey : public ObjectKey {
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

	static void triggerDataChange(Defaults defaults, const QWriteLocker &);

public Q_SLOTS:
	void setUploadingEnabled(bool uploading);
	void clearUploads();

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
	struct Q_DATASYNC_EXPORT UploadInfo {
		ObjectKey key;
		quint64 version;
		bool isDelete;
	};

	static const int UploadLimit; //TODO get from server instead

	LocalStore *_store;
	bool _uploadingEnabled;
	QHash<CachedObjectKey, UploadInfo> _activeUploads;
	quint32 _changeEstimate;
	//TODO timeout?
};

uint Q_DATASYNC_EXPORT qHash(const ChangeController::CachedObjectKey &key, uint seed);

}

Q_DECLARE_METATYPE(QtDataSync::ChangeController::ChangeInfo)

#endif // CHANGECONTROLLER_P_H
