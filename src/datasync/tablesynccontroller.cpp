#include "tablesynccontroller.h"
#include "tablesynccontroller_p.h"
#include "tabledatamodel_p.h"
using namespace QtDataSync;
namespace sph = std::placeholders;

QString TableSyncController::table() const
{
	Q_D(const TableSyncController);
	return d->table;
}

bool TableSyncController::isValid() const
{
	Q_D(const TableSyncController);
	return d->model;
}

TableSyncController::SyncState TableSyncController::syncState() const
{
	Q_D(const TableSyncController);
	return d->model ?
		d->model->state() :
		SyncState::Invalid;
}

bool TableSyncController::isLiveSyncEnabled() const
{
	Q_D(const TableSyncController);
	return d->model ?
		d->model->isLiveSyncEnabled() :
		false;
}

void TableSyncController::start()
{
	Q_D(TableSyncController);
	if (d->model)
		d->model->start();
}

void TableSyncController::stop()
{
	Q_D(TableSyncController);
	if (d->model)
		d->model->stop();
}

void TableSyncController::triggerSync(bool reconnectLiveSync)
{
	Q_D(TableSyncController);
	if (d->model)
		d->model->triggerSync(reconnectLiveSync);
}

void TableSyncController::setLiveSyncEnabled(bool liveSyncEnabled)
{
	Q_D(TableSyncController);
	if (d->model)
		d->model->setLiveSyncEnabled(liveSyncEnabled);
}

TableSyncController::TableSyncController(QString &&table, TableDataModel *model, QObject *parent) :
	QObject{*new TableSyncControllerPrivate{}, parent}
{
	Q_D(TableSyncController);
	d->table = std::move(table);
	d->model = model;

	connect(d->model, &TableDataModel::destroyed,
			this, std::bind(&TableSyncController::validChanged, this, false, QPrivateSignal{}));
	connect(d->model, &TableDataModel::stateChanged,
			this, std::bind(&TableSyncController::syncStateChanged, this, sph::_1, QPrivateSignal{}));
	connect(d->model, &TableDataModel::liveSyncEnabledChanged,
			this, std::bind(&TableSyncController::liveSyncEnabledChanged, this, sph::_1, QPrivateSignal{}));
}
