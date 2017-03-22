#ifndef DATAMERGER_P_H
#define DATAMERGER_P_H

#include "qdatasync_global.h"
#include "datamerger.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT DataMergerPrivate
{
public:
	DataMerger::SyncPolicy syncPolicy;
	DataMerger::MergePolicy mergePolicy;

	DataMergerPrivate();
};

}

#endif // DATAMERGER_P_H
