package de.skycoder42.qtdatasync;

import android.os.Build;

import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.SharedPreferences;

import android.preference.PreferenceManager;

import android.util.Log;

import de.skycoder42.qtservice.AndroidService;

//! A helper class used by the android background sync service to restart the service after a reboot
public class SyncBootReceiver extends BroadcastReceiver {
	//! The key used to store the name of the service to be started
	public static final String ServiceNameKey = "de.skycoder42.qtdatasync.SyncBootReceiver.key.serviceName";
	//! The key used to store the synchronization interval of the service to be started
	public static final String IntervalKey = "de.skycoder42.qtdatasync.SyncBootReceiver.key.interval";

	//! @inherit{android.content.BroadcastReceiver.onReceive}
	@Override
	public void onReceive(Context context, Intent intent) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
		if(!prefs.contains(ServiceNameKey))
			return;

		Intent startIntent = new Intent("de.skycoder42.qtdatasync.registersync")
			.setClassName(context, prefs.getString(ServiceNameKey, AndroidService.class.getName()))
			.putExtra(IntervalKey, prefs.getLong(IntervalKey, 60));
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
			context.startForegroundService(startIntent);
		else
			context.startService(startIntent);
	}

	/*! @brief Prepares a service for beeing rescheduled by registering it with the receiver
	 *
	 * @param context The android context of the activity/service making this call
	 * @param serviceName The JAVA-Class name of the service to be run for synchronization
	 * @param interval The synchronization interval the service should run at
	 *
	 * Internally called by QtDataSync::AndroidSyncControl to ensure a sync service gets
	 * rescheduled after a system reboot
	 */
	public static void prepareRegistration(Context context, String serviceName, long interval) {
		PreferenceManager.getDefaultSharedPreferences(context).edit()
			.putString(ServiceNameKey, serviceName)
			.putLong(IntervalKey, interval)
			.apply();
	}

	/*! @brief Remove any service registrations for rescheduling after a reboot
	 *
	 * @param context The android context of the activity/service making this call
	 *
	 * Internally called by QtDataSync::AndroidSyncControl disable rescheduling of
	 * the sync service in case background sync gets disabled
	 */
	public static void clearRegistration(Context context) {
		PreferenceManager.getDefaultSharedPreferences(context).edit()
			.remove(ServiceNameKey)
			.remove(IntervalKey)
			.apply();
	}
}
