#include "androidsynccontrol.h"
#include "androidsynccontrol_p.h"
#include "androidbackgroundservice.h"

#include <QtAndroidExtras/QAndroidIntent>
#include <QtAndroidExtras/QtAndroid>
#include <QtAndroidExtras/QAndroidJniEnvironment>
using namespace QtDataSync;

AndroidSyncControl::AndroidSyncControl() :
	d{new AndroidSyncControlData{}}
{}

AndroidSyncControl::AndroidSyncControl(const AndroidSyncControl &other) = default;

AndroidSyncControl::AndroidSyncControl(AndroidSyncControl &&other) noexcept = default;

AndroidSyncControl &AndroidSyncControl::operator=(const AndroidSyncControl &other) = default;

AndroidSyncControl &AndroidSyncControl::operator=(AndroidSyncControl &&other) noexcept = default;

AndroidSyncControl::~AndroidSyncControl() = default;

bool AndroidSyncControl::isValid() const
{
	return !d->serviceId.isEmpty();
}

QtDataSync::AndroidSyncControl::operator bool() const
{
	return isValid();
}

bool AndroidSyncControl::operator!() const
{
	return !isValid();
}

bool AndroidSyncControl::operator==(const AndroidSyncControl &other) const
{
	return d == other.d || (
				d->serviceId == other.d->serviceId &&
				d->delay == other.d->delay);
}

bool AndroidSyncControl::operator!=(const AndroidSyncControl &other) const
{
	return d != other.d && (
				d->serviceId != other.d->serviceId ||
				d->delay != other.d->delay);
}

QString AndroidSyncControl::serviceId() const
{
	return d->serviceId;
}

qint64 AndroidSyncControl::delay() const
{
	return d->delay;
}

bool AndroidSyncControl::isEnabled() const
{
	return d->createPendingIntent(false).isValid();
}

void AndroidSyncControl::setServiceId(QString serviceId)
{
	d->serviceId = std::move(serviceId);
}

void AndroidSyncControl::setDelay(qint64 delay)
{
	d->delay = delay;
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
			alarmManager.callMethod<void>("setInexactRepeating", "(IJJLandroid/app/PendingIntent;)V",
										  RTC_WAKEUP,
										  static_cast<jlong>(QDateTime::currentDateTime().addSecs(d->delay).toMSecsSinceEpoch()),
										  static_cast<jlong>(d->delay),
										  pendingIntent.object());
		} else {
			alarmManager.callMethod<void>("cancel", "(Landroid/app/PendingIntent;)V",
										  pendingIntent.object());
		}
	}
}

// ------------- PRIVATE IMPLEMENTATION -------------

AndroidSyncControlData::AndroidSyncControlData() = default;

AndroidSyncControlData::AndroidSyncControlData(const AndroidSyncControlData &other) = default;

QAndroidJniObject AndroidSyncControlData::createPendingIntent(bool allowCreate) const
{
	QAndroidJniExceptionCleaner cleaner;
	static const auto FLAG_INCLUDE_STOPPED_PACKAGES = QAndroidJniObject::getStaticField<jint>("android/content/Intent",
																							  "FLAG_INCLUDE_STOPPED_PACKAGES");
	static const auto FLAG_UPDATE_CURRENT = QAndroidJniObject::getStaticField<jint>("android/app/PendingIntent",
																					"FLAG_UPDATE_CURRENT");
	static const auto FLAG_NO_CREATE = QAndroidJniObject::getStaticField<jint>("android/app/PendingIntent",
																			   "FLAG_NO_CREATE");

	QAndroidIntent intent {
		QtAndroid::androidContext(),
		qUtf8Printable(serviceId)
	};
	intent.handle().callObjectMethod("setAction", "(Ljava/lang/String;)Landroid/content/Intent;",
									 QAndroidJniObject::fromString(AndroidBackgroundService::BackgroundSyncAction).object());
	intent.handle().callObjectMethod("addFlags", "(I)Landroid/content/Intent;",
									 FLAG_INCLUDE_STOPPED_PACKAGES);

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
