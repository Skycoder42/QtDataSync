#include "syncdelegate.h"
#include <QtCore/QDebug>

SyncDelegate::SyncDelegate(QObject *parent) :
	IosSyncDelegate{parent}
{}

QtDataSync::IosSyncDelegate::SyncResult SyncDelegate::onSyncCompleted(QtDataSync::SyncManager::SyncState state)
{
	qInfo() << "Finished synchronization in state:" << state;
	return SyncResult::NewData;
}
