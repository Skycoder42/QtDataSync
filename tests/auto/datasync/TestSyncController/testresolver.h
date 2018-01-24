#ifndef TESTRESOLVER_H
#define TESTRESOLVER_H

#include <QObject>
#include <QtDataSync/conflictresolver.h>

#include "testdata.h"

class TestResolver : public QtDataSync::GenericConflictResolver<TestData>
{
	Q_OBJECT

public:
	explicit TestResolver(QObject *parent = nullptr);

	TestData resolveConflict(TestData data1, TestData data2, QObject *parent) const override;
};

#endif // TESTRESOLVER_H
