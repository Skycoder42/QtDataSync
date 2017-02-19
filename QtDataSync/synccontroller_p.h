#ifndef SYNCCONTROLLER_P_H
#define SYNCCONTROLLER_P_H

#include "storageengine.h"
#include "synccontroller.h"

namespace QtDataSync {

class SyncControllerPrivate
{
public:
	StorageEngine *engine;
};

}

#endif // SYNCCONTROLLER_P_H
