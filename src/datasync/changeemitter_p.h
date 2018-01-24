#ifndef QTDATASYNC_CHANGEEMITTER_P_H
#define QTDATASYNC_CHANGEEMITTER_P_H

#include <QtCore/QObject>
#include <QtCore/QJsonObject>

#include "qtdatasync_global.h"
#include "defaults.h"
#include "emitteradapter_p.h"

#include "rep_changeemitter_p_source.h"

namespace QtDataSync {

//not exported because base it not, too
class ChangeEmitter : public ChangeEmitterSource
{
	Q_OBJECT

public:
	explicit ChangeEmitter(const Defaults &defaults, QObject *parent = nullptr);

public Q_SLOTS:
	void triggerChange(QObject *origin,
					   const QtDataSync::ObjectKey &key,
					   bool deleted,
					   bool changed);
	void triggerClear(QObject *origin, const QByteArray &typeName);
	void triggerReset(QObject *origin);
	void triggerUpload() override;

Q_SIGNALS:
	void uploadNeeded();

	void dataChanged(QObject *origin, const QtDataSync::ObjectKey &key, bool deleted);
	void dataResetted(QObject *origin, const QByteArray &typeName);

protected Q_SLOTS:
	//remcon interface
	void triggerRemoteChange(const ObjectKey &key, bool deleted, bool changed) override;
	void triggerRemoteClear(const QByteArray &typeName) override;
	void triggerRemoteReset() override;

private:
	QSharedPointer<EmitterAdapter::CacheInfo> _cache;//needed to clear cache on remote changes
};

}

#endif // QTDATASYNC_CHANGEEMITTER_P_H
