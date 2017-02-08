#ifndef TESTDATA_H
#define TESTDATA_H

#include <QObject>

class TestData : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int test MEMBER test)

public:
	explicit TestData(QObject *parent = nullptr);
	explicit TestData(int test, QObject *parent = nullptr);

	int test;
};

#endif // TESTDATA_H
