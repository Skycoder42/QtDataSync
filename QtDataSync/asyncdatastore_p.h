#ifndef ASYNCDATASTORE_P_H
#define ASYNCDATASTORE_P_H

#include "asyncdatastore.h"
#include "storageengine.h"

namespace QtDataSync {

class AsyncDataStorePrivate
{
public:
	StorageEngine *engine;
};

}

#endif // ASYNCDATASTORE_P_H
