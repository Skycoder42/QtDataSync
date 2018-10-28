#ifndef QTDATASYNC_ANDROIDBACKGROUNDSERVICE_P_H
#define QTDATASYNC_ANDROIDBACKGROUNDSERVICE_P_H

#include "androidbackgroundservice.h"

#include <QtDataSync/SyncManager>

namespace QtDataSync {

class AndroidBackgroundServicePrivate
{
public:
	SyncManager *manager = nullptr;
	int startId = -1;

	bool waitFullSync = false;
};

}

#endif // QTDATASYNC_ANDROIDBACKGROUNDSERVICE_P_H
