#ifndef SYNCDELEGATE_H
#define SYNCDELEGATE_H

#include <QObject>
#include <QtDataSyncIos/IosSyncDelegate>

class SyncDelegate : public QtDataSync::IosSyncDelegate
{
	Q_OBJECT

public:
	explicit SyncDelegate(QObject *parent = nullptr);

protected:
	SyncResult onSyncCompleted(QtDataSync::SyncManager::SyncState state) override;
};

#endif // SYNCDELEGATE_H
