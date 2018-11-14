#include "syncservice.h"
#include <QtAndroid>

//! [droid_service_impl]
SyncService::SyncService(int &argc, char **argv) :
	AndroidBackgroundService{argc, argv}
{}

void SyncService::prepareSetup(QtDataSync::Setup &setup)
{
	// prepare the setup here, i.e.
	// setup.setRemoteConfiguration(QtDataSync::RemoteConfig{ ... ]);
}

void SyncService::onSyncCompleted(QtDataSync::SyncManager::SyncState state)
{
	qDebug() << "[[SYNCSERVICE]]" << "Sync completed in state:" << state;
	exitAfterSync();
}
//! [droid_service_impl]

//! [jni_create_notification]
QAndroidJniObject SyncService::createForegroundNotification()
{
	return QAndroidJniObject::callStaticObjectMethod("de/skycoder42/qtdatasync/sample/androidsync/SvcHelper",
													 "createFgNotification",
													 "(Landroid/content/Context;)Landroid/app/Notification;",
													 QtAndroid::androidService().object());
}
//! [jni_create_notification]
