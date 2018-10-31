#ifndef QTDATASYNC_ANDROIDBACKGROUNDSERVICE_H
#define QTDATASYNC_ANDROIDBACKGROUNDSERVICE_H

#include <QtCore/qscopedpointer.h>

#include <QtService/service.h>

#include <QtAndroidExtras/qandroidintent.h>

#include <QtDataSync/setup.h>
#include <QtDataSync/syncmanager.h>

#include "QtDataSyncAndroid/qtdatasyncandroid_global.h"

namespace QtDataSync {

class AndroidBackgroundServicePrivate;
class Q_DATASYNCANDROID_EXPORT AndroidBackgroundService : public QtService::Service // clazy:exclude=ctor-missing-parent-argument
{
	Q_OBJECT

	Q_PROPERTY(bool waitFullSync READ waitFullSync WRITE setWaitFullSync NOTIFY waitFullSyncChanged)

public:
	static const QString BackgroundSyncAction;
	static const QString RegisterSyncAction;
	static const int ForegroundNotificationId;

	AndroidBackgroundService(int &argc, char **argv, int = QCoreApplication::ApplicationFlags);
	~AndroidBackgroundService() override;

	bool waitFullSync() const;

public Q_SLOTS:
	void setWaitFullSync(bool waitFullSync);

Q_SIGNALS:
	void waitFullSyncChanged(bool waitFullSync, QPrivateSignal);

protected:
	virtual QString setupName() const;
	virtual void prepareSetup(Setup &setup);

	CommandResult onStart() override;
	CommandResult onStop(int &exitCode) override;

	virtual void onSyncCompleted(SyncManager::SyncState state);
	void exitAfterSync();

	virtual int onStartCommand(const QAndroidIntent &intent, int flags, int startId);
	virtual QAndroidJniObject createForegroundNotification() = 0;
	virtual bool stopSelf(int startId);

private Q_SLOTS:
	void startBackgroundSync(int startId);
	void registerSync(int startId, qint64 delay);
	void backendReady();

private:
	QScopedPointer<AndroidBackgroundServicePrivate> d;
};

}

#endif // QTDATASYNC_ANDROIDBACKGROUNDSERVICE_H
