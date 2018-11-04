#include "androidbackgroundservice.h"
#include "androidbackgroundservice_p.h"
#include "androidsynccontrol.h"

#include <QtRemoteObjects/QRemoteObjectReplica>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QAndroidJniEnvironment>
#include <QtAndroidExtras/QtAndroid>
using namespace QtDataSync;

Q_DECLARE_METATYPE(QAndroidIntent)

const QString AndroidBackgroundService::BackgroundSyncAction{QStringLiteral("de.skycoder42.qtdatasync.backgroundsync")};
const QString AndroidBackgroundService::RegisterSyncAction{QStringLiteral("de.skycoder42.qtdatasync.registersync")};
const int AndroidBackgroundService::ForegroundNotificationId = 0x2f933457;

AndroidBackgroundService::AndroidBackgroundService(int &argc, char **argv, int flags) :
	Service{argc, argv, flags},
	d{new AndroidBackgroundServicePrivate{}}
{
	addCallback("onStartCommand", &AndroidBackgroundService::onStartCommand);
}

bool AndroidBackgroundService::waitFullSync() const
{
	return d->waitFullSync;
}

void AndroidBackgroundService::setWaitFullSync(bool waitFullSync)
{
	if (d->waitFullSync == waitFullSync)
		return;

	d->waitFullSync = waitFullSync;
	emit waitFullSyncChanged(d->waitFullSync, {});
}

AndroidBackgroundService::~AndroidBackgroundService() = default;

QString AndroidBackgroundService::setupName() const
{
	return DefaultSetup;
}

void AndroidBackgroundService::prepareSetup(Setup &setup)
{
	Q_UNUSED(setup)
}

QtService::Service::CommandResult AndroidBackgroundService::onStart()
{
	return OperationCompleted;
}

QtService::Service::CommandResult AndroidBackgroundService::onStop(int &exitCode)
{
	if(d->manager) {
		delete d->manager;
		d->manager = nullptr;
	}

	Setup::removeSetup(setupName(), true); //TODO make optional (rmwait, rmnowait, norm)
	exitCode = EXIT_SUCCESS;
	return OperationCompleted;
}

void AndroidBackgroundService::onSyncCompleted(SyncManager::SyncState state)
{
	Q_UNUSED(state)
	exitAfterSync();
}

void AndroidBackgroundService::exitAfterSync()
{
	if(d->startId != -1) {
		stopSelf(d->startId);
		d->startId = -1;
	}
}

int AndroidBackgroundService::onStartCommand(const QAndroidIntent &intent, int flags, int startId)
{
	Q_UNUSED(flags)

	QAndroidJniExceptionCleaner cleaner;
	static const auto START_REDELIVER_INTENT = QAndroidJniObject::getStaticField<jint>("android/app/Service", "START_REDELIVER_INTENT");

	auto notification = createForegroundNotification();
	if(notification.isValid()) {
		QtAndroid::androidService().callMethod<void>("startForeground", "(ILandroid/app/Notification;)V",
													 static_cast<jint>(ForegroundNotificationId),
													 notification.object());
	} else
		qCritical() << "Foreground notification could not be created - Service will get an ANR and be killed in a few seconds!";

	if(intent.handle().isValid()) {
		const auto action = intent.handle().callObjectMethod("getAction", "()Ljava/lang/String;").toString();
		if(action == RegisterSyncAction) {
			const auto interval = intent.handle().callMethod<jlong>("getLongExtra", "(Ljava/lang/String;J)J",
																 QAndroidJniObject::fromString(QStringLiteral("de.skycoder42.qtdatasync.SyncBootReceiver.key.interval")).object(),
																 static_cast<jlong>(60));
			QMetaObject::invokeMethod(this, "registerSync", Qt::QueuedConnection,
									  Q_ARG(int, startId),
									  Q_ARG(qint64, interval));
		} else if(action == BackgroundSyncAction) {
			QMetaObject::invokeMethod(this, "startBackgroundSync", Qt::QueuedConnection,
									  Q_ARG(int, startId));
		}
	}

	return START_REDELIVER_INTENT;
}

bool AndroidBackgroundService::stopSelf(int startId)
{
	QAndroidJniExceptionCleaner cleaner;
	auto stopped = QtAndroid::androidService().callMethod<jboolean>("stopSelfResult", "(I)Z",
																	static_cast<jint>(startId));
	if(stopped) {
		QtAndroid::androidService().callMethod<void>("stopForeground", "(Z)V",
													 static_cast<jboolean>(true));
	}
	return stopped;
}

void AndroidBackgroundService::startBackgroundSync(int startId)
{
	if(d->startId != -1) {
		qWarning() << "Background synchronization already running! Ignoring additional sync request";
		d->startId = startId;
		return;
	}

	try {
		// create the setup
		if(!Setup::exists(setupName())) {
			Setup setup;
			prepareSetup(setup);
			setup.create(setupName());
		}

		// create the manager
		if(!d->manager)
			d->manager = new SyncManager{setupName(), this};
		d->startId = startId;

		if(d->manager->replica()->isInitialized())
			backendReady();
		else {
			connect(d->manager->replica(), &QRemoteObjectReplica::initialized,
					this, &AndroidBackgroundService::backendReady,
					Qt::UniqueConnection);
		}
	} catch (Exception &e) {
		qCritical() << e.what();
		d->startId = -1;
		stopSelf(startId);
	}
}

void AndroidBackgroundService::registerSync(int startId, qint64 interval)
{
	AndroidSyncControl syncControl;
	syncControl.setServiceId(QtAndroid::androidService()
							 .callObjectMethod("getClass", "()Ljava/lang/Class;")
							 .callObjectMethod("getName", "()Ljava/lang/String;")
							 .toString());
	syncControl.setInterval(interval);
	syncControl.setEnabled(true);
	if(!syncControl.isEnabled()) {
		qWarning() << "Failed to register service" << syncControl.serviceId()
				   << "for background synchronization!";
	}

	startBackgroundSync(startId);
}

void AndroidBackgroundService::backendReady()
{
	const auto syncFn = std::bind(&AndroidBackgroundService::onSyncCompleted, this, std::placeholders::_1);
	if(d->waitFullSync)
		d->manager->runOnSynchronized(syncFn, true);
	else
		d->manager->runOnDownloaded(syncFn, true);
}

// ------------- PRIVATE IMPLEMENTATION -------------
