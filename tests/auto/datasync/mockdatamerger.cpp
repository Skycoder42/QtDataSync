#include "mockdatamerger.h"

MockDataMerger::MockDataMerger(QObject *parent) :
	DataMerger(parent),
	mergeCount(0)
{}

QJsonObject MockDataMerger::merge(QJsonObject local, QJsonObject remote)
{
	mergeCount++;
	return DataMerger::merge(local, remote);
}
