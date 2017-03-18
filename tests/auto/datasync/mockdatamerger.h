#ifndef MOCKDATAMERGER_H
#define MOCKDATAMERGER_H

#include <datamerger.h>

class MockDataMerger : public QtDataSync::DataMerger
{
	Q_OBJECT

public:
	MockDataMerger(QObject *parent = nullptr);

	QJsonObject merge(QJsonObject local, QJsonObject remote) override;

public:
	int mergeCount;
};

#endif // MOCKDATAMERGER_H
