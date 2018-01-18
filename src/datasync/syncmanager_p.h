#ifndef SYNCMANAGER_P_H
#define SYNCMANAGER_P_H

#include <QtCore/QPointer>

#include "qtdatasync_global.h"
#include "exchangeengine_p.h"

#include "rep_syncmanager_p_source.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT SyncManagerPrivate : public SyncManagerPrivateSource
{
	Q_OBJECT

public:
	SyncManagerPrivate(ExchangeEngine *engineParent);

	bool syncEnabled() const override;
	SyncManager::SyncState syncState() const override;
	qreal syncProgress() const override;
	QString lastError() const override;

	void setSyncEnabled(bool syncEnabled) override;

public Q_SLOTS:
	void synchronize() override;
	void reconnect() override;
	void runOnState(const QUuid &id, bool downloadOnly, bool triggerSync) override;

private:
	QPointer<ExchangeEngine> _engine;
};

}

#endif // SYNCMANAGER_P_H
