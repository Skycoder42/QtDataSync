#include "androidsynccontrol.h"
#include "androidsynccontrol_p.h"
#include "androidbackgroundservice.h"

#include <QtAndroidExtras/QtAndroid>
#include <QtAndroidExtras/QAndroidJniEnvironment>
using namespace QtDataSync;
using namespace std::chrono;

AndroidSyncControl::AndroidSyncControl(QObject *parent) :
	QObject{parent},
	d{new AndroidSyncControlPrivate{}}
{}

AndroidSyncControl::AndroidSyncControl(QString serviceId, QObject *parent) :
	AndroidSyncControl{parent}
{
	d->serviceId = std::move(serviceId);
}

AndroidSyncControl::~AndroidSyncControl() = default;

bool AndroidSyncControl::isValid() const
{
	return !d->serviceId.isEmpty();
}

QString AndroidSyncControl::serviceId() const
{
	return d->serviceId;
}

qint64 AndroidSyncControl::delay() const
{
	return d->delay.count();
}

std::chrono::minutes AndroidSyncControl::delayMinutes() const
{
	return d->delay;
}

bool AndroidSyncControl::isEnabled() const
{
	return d->createPendingIntent(false).isValid();
}

void AndroidSyncControl::setServiceId(QString serviceId)
{
	if(d->serviceId == serviceId)
		return;

	d->serviceId = std::move(serviceId);
	emit serviceIdChanged(d->serviceId, {});
}

void AndroidSyncControl::setDelay(qint64 delay)
{
	setDelay(minutes{delay});
}

void AndroidSyncControl::setDelay(std::chrono::minutes delay)
{
	if(d->delay == delay)
		return;

	d->delay = delay;
	emit delayChanged(d->delay.count(), {});
}

void AndroidSyncControl::setEnabled(bool enabled)
{
	if(enabled == isEnabled())
		return;

	auto pendingIntent = d->createPendingIntent(true);
	if(!pendingIntent.isValid())
		return;

	QAndroidJniExceptionCleaner cleaner;
	static const auto RTC_WAKEUP = QAndroidJniObject::getStaticField<jint>("android/app/AlarmManager",
																		   "RTC_WAKEUP");
	const auto ALARM_SERVICE = QAndroidJniObject::getStaticObjectField("android/content/Context",
																	   "ALARM_SERVICE",
																	   "Ljava/lang/String;");

	auto alarmManager = QtAndroid::androidContext().callObjectMethod("getSystemService",
																	 "(Ljava/lang/String;)Ljava/lang/Object;",
																	 ALARM_SERVICE.object());
	if(alarmManager.isValid()) {
		if(enabled) {
			auto delta = duration_cast<milliseconds>(d->delay).count();
			alarmManager.callMethod<void>("setInexactRepeating", "(IJJLandroid/app/PendingIntent;)V",
										  RTC_WAKEUP,
										  static_cast<jlong>(QDateTime::currentDateTime().addMSecs(delta).toMSecsSinceEpoch()),
										  static_cast<jlong>(delta),
										  pendingIntent.object());
			QAndroidJniObject::callStaticMethod<void>("de/skycoder42/qtdatasync/SyncBootReceiver",
													  "prepareRegistration",
													  "(Landroid/content/Context;Ljava/lang/String;J)V",
													  QtAndroid::androidContext().object(),
													  QAndroidJniObject::fromString(d->serviceId).object(),
													  static_cast<jlong>(d->delay.count()));
		} else {
			alarmManager.callMethod<void>("cancel", "(Landroid/app/PendingIntent;)V",
										  pendingIntent.object());
			QAndroidJniObject::callStaticMethod<void>("de/skycoder42/qtdatasync/SyncBootReceiver",
													  "clearRegistration",
													  "(Landroid/content/Context;)V",
													  QtAndroid::androidContext().object());
		}
	}
}

bool AndroidSyncControl::triggerSyncNow()
{
	auto intent = d->createIntent();
	if(!intent.handle().isValid())
		return false;

	QAndroidJniExceptionCleaner cleaner;
	auto res = QtAndroid::androidContext().callObjectMethod(QtAndroid::androidSdkVersion() >= 26 ?
																"startForegroundService" :
																"startService",
															"(Landroid/content/Intent;)Landroid/content/ComponentName;",
															intent.handle().object());
	return res.isValid();
}

// ------------- PRIVATE IMPLEMENTATION -------------

QAndroidIntent AndroidSyncControlPrivate::createIntent() const
{
	QAndroidJniExceptionCleaner cleaner;
	static const auto FLAG_INCLUDE_STOPPED_PACKAGES = QAndroidJniObject::getStaticField<jint>("android/content/Intent",
																							  "FLAG_INCLUDE_STOPPED_PACKAGES");

	QAndroidIntent intent {
		QtAndroid::androidContext(),
		qUtf8Printable(serviceId)
	};
	intent.handle().callObjectMethod("setAction", "(Ljava/lang/String;)Landroid/content/Intent;",
									 QAndroidJniObject::fromString(AndroidBackgroundService::BackgroundSyncAction).object());
	intent.handle().callObjectMethod("addFlags", "(I)Landroid/content/Intent;",
									 FLAG_INCLUDE_STOPPED_PACKAGES);
	return intent;
}

QAndroidJniObject AndroidSyncControlPrivate::createPendingIntent(bool allowCreate) const
{
	auto intent = createIntent();
	if(!intent.handle().isValid())
		return {};

	QAndroidJniExceptionCleaner cleaner;
	static const auto FLAG_UPDATE_CURRENT = QAndroidJniObject::getStaticField<jint>("android/app/PendingIntent",
																					"FLAG_UPDATE_CURRENT");
	static const auto FLAG_NO_CREATE = QAndroidJniObject::getStaticField<jint>("android/app/PendingIntent",
																			   "FLAG_NO_CREATE");

	return QAndroidJniObject::callStaticObjectMethod("android/app/PendingIntent",
													 QtAndroid::androidSdkVersion() >= 26 ?
														 "getForegroundService" :
														 "getService",
													 "(Landroid/content/Context;ILandroid/content/Intent;I)Landroid/app/PendingIntent;",
													 QtAndroid::androidContext().object(),
													 static_cast<jint>(0),
													 intent.handle().object(),
													 allowCreate ? FLAG_UPDATE_CURRENT : FLAG_NO_CREATE);
}
