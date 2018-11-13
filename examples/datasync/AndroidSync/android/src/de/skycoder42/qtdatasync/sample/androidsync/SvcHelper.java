package de.skycoder42.qtdatasync.sample.androidsync;

import android.os.Build;

import android.content.Context;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;

import android.graphics.BitmapFactory;

import android.support.v4.app.NotificationCompat;

public class SvcHelper {
	private static final String ForegroundChannelId = "de.skycoder42.qtdatasync.sample.androidsync.notify";

	//! [create_notification]
	public static Notification createFgNotification(Context context) {
		return new NotificationCompat.Builder(context, ForegroundChannelId)
			.setContentTitle("AndroidSync Synchronization Service")
			.setContentText("Synchronizing…")
			.setContentInfo("AndroidSync")
			.setLargeIcon(BitmapFactory.decodeResource(context.getResources(), R.drawable.icon))
			.setSmallIcon(R.drawable.icon)
			.setLocalOnly(true)
			.setOngoing(true)
			.setCategory(NotificationCompat.CATEGORY_SERVICE)
			.build();
	}
	//! [create_notification]

	public static void registerNotificationChannel(Context context) {
		if(Build.VERSION.SDK_INT < Build.VERSION_CODES.O)
			return;

		NotificationManager manager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);

		//create foreground channel
		NotificationChannel foreground = new NotificationChannel(ForegroundChannelId,
				"Synchronization Service",
				NotificationManager.IMPORTANCE_MIN);
		foreground.setDescription("Is shown when the application synchronizes");
		foreground.enableLights(false);
		foreground.enableVibration(false);
		foreground.setShowBadge(false);
		manager.createNotificationChannel(foreground);
	}
}
