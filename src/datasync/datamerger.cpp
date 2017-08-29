#include "datamerger.h"
#include "datamerger_p.h"

using namespace QtDataSync;

DataMerger::DataMerger(QObject *parent) :
	QObject(parent),
	d(new DataMergerPrivate())
{}

DataMerger::~DataMerger() {}

void DataMerger::initialize(Defaults *) {}

void DataMerger::finalize() {}

DataMerger::SyncPolicy DataMerger::syncPolicy() const
{
	return d->syncPolicy;
}

DataMerger::MergePolicy DataMerger::mergePolicy() const
{
	return d->mergePolicy;
}

QJsonObject DataMerger::merge(QJsonObject local, QJsonObject remote)
{
	Q_UNUSED(remote);
	return local;
}

void DataMerger::setSyncPolicy(DataMerger::SyncPolicy syncPolicy)
{
	d->syncPolicy = syncPolicy;
}

void DataMerger::setMergePolicy(DataMerger::MergePolicy mergePolicy)
{
	d->mergePolicy = mergePolicy;
}



DataMergerPrivate::DataMergerPrivate() :
	syncPolicy(DataMerger::PreferUpdated),
	mergePolicy(DataMerger::KeepLocal)
{}



DataMerger2::DataMerger2(QObject *parent) :
	DataMerger(parent)
{}

QJsonObject DataMerger2::merge(QJsonObject local, QJsonObject remote, const QByteArray &typeName)
{
	Q_UNUSED(remote);
	Q_UNUSED(typeName);
	return local;
}

QJsonObject DataMerger2::merge(QJsonObject local, QJsonObject remote)
{
	Q_UNUSED(local);
	Q_UNUSED(remote);
	Q_UNREACHABLE();
	return {};
}
