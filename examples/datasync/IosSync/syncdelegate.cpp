#include "syncdelegate.h"
#include <QtCore/QDebug>

SyncDelegate::SyncDelegate(QObject *parent) :
	IosSyncDelegate{parent}
{}

//! [delegate_src]
QtDataSync::IosSyncDelegate::SyncResult SyncDelegate::onSyncCompleted(QtDataSync::SyncManager::SyncState state)
{
	qInfo() << "Finished synchronization in state:" << state;
	return IosSyncDelegate::onSyncCompleted(state);
}
//! [delegate_src]
