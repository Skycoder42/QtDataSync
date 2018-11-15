#ifndef SYNCDELEGATE_H
#define SYNCDELEGATE_H

#include <QObject>
#include <QtDataSyncIos/IosSyncDelegate>

//! [delegate_hdr]
class SyncDelegate : public QtDataSync::IosSyncDelegate
{
	Q_OBJECT

public:
	explicit SyncDelegate(QObject *parent = nullptr);

protected:
	SyncResult onSyncCompleted(QtDataSync::SyncManager::SyncState state) override;
};
//! [delegate_hdr]

#endif // SYNCDELEGATE_H
