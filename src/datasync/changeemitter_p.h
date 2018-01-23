#ifndef CHANGEEMITTER_P_H
#define CHANGEEMITTER_P_H

#include <QtCore/QObject>
#include <QtCore/QJsonObject>

#include "qtdatasync_global.h"

#include "rep_changeemitter_p_source.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ChangeEmitter : public ChangeEmitterSource
{
	Q_OBJECT

public:
	explicit ChangeEmitter(QObject *parent = nullptr);

public Q_SLOTS:
	void triggerChange(QObject *origin,
					   const QtDataSync::ObjectKey &key,
					   const QJsonObject data,
					   int size,
					   bool changed);
	void triggerClear(QObject *origin, const QByteArray &typeName);
	void triggerReset(QObject *origin);

Q_SIGNALS:
	void uploadNeeded();

	void dataChanged(QObject *origin,
					 const QtDataSync::ObjectKey &key,
					 bool deleted,
					 const QJsonObject data,
					 int size);
	void dataResetted(QObject *origin, const QByteArray &typeName);

protected Q_SLOTS:
	//remcon interface
	void triggerRemoteChange(const ObjectKey &key, bool deleted, bool changed) override;
	void triggerRemoteClear(const QByteArray &typeName) override;
	void triggerRemoteReset() override;
};

}

#endif // CHANGEEMITTER_P_H
