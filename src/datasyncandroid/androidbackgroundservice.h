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
//! An extension of QtService to create a synchronization service for android
class Q_DATASYNCANDROID_EXPORT AndroidBackgroundService : public QtService::Service // clazy:exclude=ctor-missing-parent-argument
{
	Q_OBJECT

	//! Specify whether the service should wait for a full synchronization or only the download
	Q_PROPERTY(bool waitFullSync READ waitFullSync WRITE setWaitFullSync NOTIFY waitFullSyncChanged)

public:
	//! The Intent Action used to start the background synchronization
	static const QString BackgroundSyncAction;
	//! The Intent Action used to register the service for background synchronization
	static const QString RegisterSyncAction;
	//! The ID of the Foreground notification shown by the service while synchronizing
	static const int ForegroundNotificationId;

	//! Default constructor
	AndroidBackgroundService(int &argc, char **argv, int = QCoreApplication::ApplicationFlags);
	~AndroidBackgroundService() override;

	//! @readAcFn{AndroidBackgroundService::waitFullSync}
	bool waitFullSync() const;

public Q_SLOTS:
	//! @writeAcFn{AndroidBackgroundService::waitFullSync}
	void setWaitFullSync(bool waitFullSync);

Q_SIGNALS:
	//! @notifyAcFn{AndroidBackgroundService::waitFullSync}
	void waitFullSyncChanged(bool waitFullSync, QPrivateSignal);

protected:
	//! Returns the name of the setup to be synchronized
	virtual QString setupName() const;
	//! Prepares the setup to be synchronized
	virtual void prepareSetup(Setup &setup);

	//! @inherit{QtService::Service::onStart}
	CommandResult onStart() override;
	//! @inherit{QtService::Service::onStop}
	CommandResult onStop(int &exitCode) override;

	//! Is called by the service as soon the the synchronization has been completed
	virtual void onSyncCompleted(SyncManager::SyncState state);
	//! Tells the service to quit itself after a synchronization
	void exitAfterSync();

	//! Is called on the android thread to deliver the start command
	virtual int onStartCommand(const QAndroidIntent &intent, int flags, int startId);
	//! Is called on the android thread to create a foreground notification
	virtual QAndroidJniObject createForegroundNotification() = 0;
	//! Is called by the service to stop itself once finished, based on the id it was started with
	virtual bool stopSelf(int startId);

private Q_SLOTS:
	void startBackgroundSync(int startId);
	void registerSync(int startId, qint64 interval);
	void backendReady();

private:
	QScopedPointer<AndroidBackgroundServicePrivate> d;
};

}

#endif // QTDATASYNC_ANDROIDBACKGROUNDSERVICE_H
