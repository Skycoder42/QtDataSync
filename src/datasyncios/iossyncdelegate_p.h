#ifndef QTDATASYNC_IOSSYNCDELEGATE_P_H
#define QTDATASYNC_IOSSYNCDELEGATE_P_H

#include "iossyncdelegate.h"

#include <QtCore/QPointer>
#include <QtCore/QSettings>

namespace QtDataSync {

class IosSyncDelegatePrivate
{
	friend class IosSyncDelegate;

public:
	static void performFetch(std::function<void(IosSyncDelegate::SyncResult)> callback);

private:
	static const QString SyncStateGroup;
	static const QString SyncDelayKey;
	static const QString SyncEnabledKey;
	static const QString SyncWaitKey;

	static QPointer<IosSyncDelegate> delegateInstance;

	IosSyncDelegatePrivate(QSettings *settings, IosSyncDelegate *q_ptr);

	IosSyncDelegate *q;
	bool enabled = true;
	std::chrono::minutes delay{60};
	bool waitFullSync = false;

	QSettings *settings;
	QtDataSync::SyncManager *manager = nullptr;
	bool allowUpdateSync = false;

	std::function<void(IosSyncDelegate::SyncResult)> currentCallback;

	void updateSyncInterval();
	void storeState();
	void onSyncDone(SyncManager::SyncState state);
};

}

#endif // QTDATASYNC_IOSSYNCDELEGATE_P_H
