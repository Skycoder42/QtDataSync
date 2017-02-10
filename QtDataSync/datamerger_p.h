#ifndef DATAMERGER_P_H
#define DATAMERGER_P_H

#include "datamerger.h"

namespace QtDataSync {

class DataMergerPrivate
{
public:
	DataMerger::SyncPolicy syncPolicy;
	DataMerger::MergePolicy mergePolicy;
	bool repeatFailed;

	DataMergerPrivate();
};

}

#endif // DATAMERGER_P_H
