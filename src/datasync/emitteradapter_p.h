#ifndef QTDATASYNC_EMITTERADAPTER_P_H
#define QTDATASYNC_EMITTERADAPTER_P_H

#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>
#include <QtCore/QCache>

#include "qtdatasync_global.h"
#include "objectkey.h"
#include "defaults.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT EmitterAdapter : public QObject
{
	Q_OBJECT

public:
	struct Q_DATASYNC_EXPORT CacheInfo {
		QReadWriteLock lock;
		QCache<ObjectKey, QJsonObject> cache;

		CacheInfo(int maxSize);
	};

	explicit EmitterAdapter(QObject *changeEmitter,
							QSharedPointer<CacheInfo> cacheInfo,
							QObject *origin = nullptr);

	void triggerChange(const QtDataSync::ObjectKey &key, bool deleted, bool changed);
	void triggerClear(const QByteArray &typeName, const QStringList &ids);
	void triggerReset();
	void triggerUpload();

	void putCached(const ObjectKey &key, const QJsonObject &data, int costs);
	void putCached(const QList<ObjectKey> &keys, const QList<QJsonObject> &data, const QList<int> &costs);
	bool getCached(const ObjectKey &key, QJsonObject &data);
	bool dropCached(const ObjectKey &key);
	void dropCached(const QByteArray &typeName, const QStringList &ids);
	void dropCached();

Q_SIGNALS:
	void dataChanged(const QtDataSync::ObjectKey &key, bool deleted);
	void dataResetted();

private Q_SLOTS:
	void dataChangedImpl(QObject *origin, const QtDataSync::ObjectKey &key, bool deleted);
	void dataResettedImpl(QObject *origin);
	void remoteDataChangedImpl(const QtDataSync::ObjectKey &key, bool deleted);
	void remoteDataResettedImpl();

private:
	bool _isPrimary;
	QObject *_emitterBackend;
	QSharedPointer<CacheInfo> _cache;
};

}

Q_DECLARE_METATYPE(QSharedPointer<QtDataSync::EmitterAdapter::CacheInfo>)

#endif // QTDATASYNC_EMITTERADAPTER_P_H
