#include "syncservice.h"
#include <QtAndroid>

SyncService::SyncService(int &argc, char **argv) :
	AndroidBackgroundService{argc, argv}
{}

void SyncService::onSyncCompleted(QtDataSync::SyncManager::SyncState state)
{
	qDebug() << "[[SYNCSERVICE]]" << "Sync completed in state:" << state;
	exitAfterSync();
}

//! [jni_create_notification]
QAndroidJniObject SyncService::createForegroundNotification()
{
	return QAndroidJniObject::callStaticObjectMethod("de/skycoder42/qtdatasync/sample/androidsync/SvcHelper",
													 "createFgNotification",
													 "(Landroid/content/Context;)Landroid/app/Notification;",
													 QtAndroid::androidService().object());
}
//! [jni_create_notification]
