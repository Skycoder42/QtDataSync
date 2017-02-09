#include "datamerger.h"
using namespace QtDataSync;

DataMerger::DataMerger(QObject *parent) :
	QObject(parent)
{}

void DataMerger::initialize() {}

void DataMerger::finalize() {}

DataMerger::SyncPolicy DataMerger::syncPolicy() const
{
	return _syncPolicy;
}

DataMerger::MergePolicy DataMerger::mergePolicy() const
{
	return _mergePolicy;
}

QJsonObject DataMerger::merge(QJsonObject local, QJsonObject)
{
	return local;
}

void DataMerger::setSyncPolicy(DataMerger::SyncPolicy syncPolicy)
{
	_syncPolicy = syncPolicy;
}

void DataMerger::setMergePolicy(DataMerger::MergePolicy mergePolicy)
{
	_mergePolicy = mergePolicy;
}
