#ifndef SYNCCONTROLLER_P_H
#define SYNCCONTROLLER_P_H

#include "qdatasync_global.h"
#include "synccontroller.h"
#include "storageengine_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT SyncControllerPrivate
{
public:
	StorageEngine *engine;
};

}

#endif // SYNCCONTROLLER_P_H
