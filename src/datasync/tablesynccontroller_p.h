#ifndef QTDATASYNC_TABLESYNCCONTROLLER_P_H
#define QTDATASYNC_TABLESYNCCONTROLLER_P_H

#include "tablesynccontroller.h"
#include "engine.h"
#include "engine_p.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT TableSyncControllerPrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(TableSyncController)

public:
	using SyncState = TableSyncController::SyncState;

	QString table;
	QPointer<TableDataModel> model;
};

}

#endif // QTDATASYNC_TABLESYNCCONTROLLER_P_H
