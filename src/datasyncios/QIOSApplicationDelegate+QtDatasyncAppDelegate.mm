#import "QIOSApplicationDelegate+QtDatasyncAppDelegate_p.h"
#import "qtdatasyncappdelegate_capi_p.h"
#include <QtCore/QDebug>
#include "iossyncdelegate_p.h"

@implementation QIOSApplicationDelegate (QtDatasyncAppDelegate)

-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	qDebug() << "initialized QtDatasyncAppDelegate";
	return YES;
}

-(void)application:(UIApplication *)application performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler
{
	QtDataSync::IosSyncDelegatePrivate::performFetch([=](QtDataSync::IosSyncDelegate::SyncResult result) {
		switch(result) {
		case QtDataSync::IosSyncDelegate::SyncResult::NewData:
			completionHandler(UIBackgroundFetchResultNewData);
			break;
		case QtDataSync::IosSyncDelegate::SyncResult::NoData:
			completionHandler(UIBackgroundFetchResultNoData);
			break;
		case QtDataSync::IosSyncDelegate::SyncResult::Error:
			completionHandler(UIBackgroundFetchResultFailed);
			break;
		default:
			Q_UNREACHABLE();
			break;
		}
	});
}



void QtDatasyncAppDelegateInitialize()
{
	QIOSApplicationDelegate *appDelegate = (QIOSApplicationDelegate *)[[UIApplication sharedApplication] delegate];
	Q_ASSERT_X(appDelegate, Q_FUNC_INFO, "Failed to initialize QIOSApplicationDelegate (QtDatasyncAppDelegate)");
}

void setSyncInterval(double delaySeconds)
{
	if(delaySeconds == -1.0)
		[[UIApplication sharedApplication] setMinimumBackgroundFetchInterval:UIApplicationBackgroundFetchIntervalNever];
	else
		[[UIApplication sharedApplication] setMinimumBackgroundFetchInterval:delaySeconds];
}

@end
