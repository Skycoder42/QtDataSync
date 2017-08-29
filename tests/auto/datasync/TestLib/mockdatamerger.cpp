#include "mockdatamerger.h"

MockDataMerger::MockDataMerger(QObject *parent) :
	DataMerger2(parent),
	mergeCount(0)
{}

QJsonObject MockDataMerger::merge(QJsonObject local, QJsonObject remote, const QByteArray &typeName)
{
	mergeCount++;
	return DataMerger2::merge(local, remote, typeName);
}
