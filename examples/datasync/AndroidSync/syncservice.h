#ifndef SYNCSERVICE_H
#define SYNCSERVICE_H

#include <QtDataSyncAndroid/AndroidBackgroundService>

//! [droid_service_hdr]
class SyncService : public QtDataSync::AndroidBackgroundService
{
	Q_OBJECT

public:
	explicit SyncService(int &argc, char **argv);

protected:
	void prepareSetup(QtDataSync::Setup &setup) override;
	void onSyncCompleted(QtDataSync::SyncManager::SyncState state) override;
	QAndroidJniObject createForegroundNotification() override;
};
//! [droid_service_hdr]

#endif // SYNCSERVICE_H
