#include "testdata.h"

TestData::TestData(QObject *parent) :
	TestData(0, parent)
{

}

TestData::TestData(int test, QObject *parent) :
	QObject(parent),
	test(test)
{}
