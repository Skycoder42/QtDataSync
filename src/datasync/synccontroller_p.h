#ifndef QTDATASYNC_SYNCCONTROLLER_P_H
#define QTDATASYNC_SYNCCONTROLLER_P_H

#include "qtdatasync_global.h"
#include "synccontroller.h"
#include "storageengine_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT SyncControllerPrivate
{
public:
	StorageEngine *engine;
};

}

#endif // QTDATASYNC_SYNCCONTROLLER_P_H
