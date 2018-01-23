#ifndef EMITTERADAPTER_P_H
#define EMITTERADAPTER_P_H

#include <QtCore/QObject>

#include "qtdatasync_global.h"
#include "objectkey.h"
#include "defaults.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT EmitterAdapter : public QObject
{
	Q_OBJECT

public:
	explicit EmitterAdapter(QObject *changeEmitter,
							QObject *origin = nullptr);

	void triggerChange(const QtDataSync::ObjectKey &key,
					   const QJsonObject data,
					   int size,
					   bool changed);
	void triggerClear(const QByteArray &typeName);
	void triggerReset();

Q_SIGNALS:
	void dataChanged(const QtDataSync::ObjectKey &key,
					 bool deleted,
					 const QJsonObject data,
					 int size);
	void dataResetted(const QByteArray &typeName);

private Q_SLOTS:
	void dataChangedImpl(QObject *origin,
						 const QtDataSync::ObjectKey &key,
						 bool deleted,
						 const QJsonObject data,
						 int size);
	void dataResettedImpl(QObject *origin, const QByteArray &typeName);
	void remoteDataChangedImpl(const QtDataSync::ObjectKey & key, bool deleted);
	void remoteDataResettedImpl(const QByteArray & typeName);

private:
	bool _isPrimary;
	QObject *_emitterBackend;
};

}

#endif // EMITTERADAPTER_P_H
