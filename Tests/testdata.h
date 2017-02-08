#ifndef TESTDATA_H
#define TESTDATA_H

#include <QObject>

class TestData : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int test MEMBER test)

public:
	Q_INVOKABLE TestData(QObject *parent = nullptr);
	TestData(int test, QObject *parent = nullptr);

	int test;
};

#endif // TESTDATA_H
