package de.skycoder42.qtdatasync;

import android.os.Build;

import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.SharedPreferences;

import android.preference.PreferenceManager;

import android.util.Log;

import de.skycoder42.qtservice.AndroidService;

public class SyncBootReceiver extends BroadcastReceiver {
	public static final String ServiceNameKey = "de.skycoder42.qtdatasync.SyncBootReceiver.key.serviceName";
	public static final String IntervalKey = "de.skycoder42.qtdatasync.SyncBootReceiver.key.interval";

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

	public static void prepareRegistration(Context context, String serviceName, long interval) {
		PreferenceManager.getDefaultSharedPreferences(context).edit()
			.putString(ServiceNameKey, serviceName)
			.putLong(IntervalKey, interval)
			.apply();
		setComponentState(context, true);
	}

	public static void clearRegistration(Context context) {
		PreferenceManager.getDefaultSharedPreferences(context).edit()
			.remove(ServiceNameKey)
			.remove(IntervalKey)
			.apply();
		setComponentState(context, false);
	}

	protected static void setComponentState(Context context, boolean enabled) {
		Log.d(SyncBootReceiver.class.getName(), "setComponentState not implemented");
	}
}
