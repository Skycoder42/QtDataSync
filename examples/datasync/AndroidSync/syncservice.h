#ifndef SYNCSERVICE_H
#define SYNCSERVICE_H

#include <QtDataSyncAndroid/AndroidBackgroundService>

class SyncService : public QtDataSync::AndroidBackgroundService
{
	Q_OBJECT

public:
	explicit SyncService(int &argc, char **argv);

protected:
	void onSyncCompleted(QtDataSync::SyncManager::SyncState state) override;
	QAndroidJniObject createForegroundNotification() override;
};

#endif // SYNCSERVICE_H
