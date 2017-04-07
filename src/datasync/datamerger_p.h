#ifndef QTDATASYNC_DATAMERGER_P_H
#define QTDATASYNC_DATAMERGER_P_H

#include "qtdatasync_global.h"
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

#endif // QTDATASYNC_DATAMERGER_P_H
