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

QJsonObject DataMerger::merge(QJsonObject local, QJsonObject)
{
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
