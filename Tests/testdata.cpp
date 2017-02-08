#include "testdata.h"

TestData::TestData(QObject *parent) :
	TestData(0, {}, parent)
{}

TestData::TestData(int id, QString text, QObject *parent) :
	QObject(parent),
	id(id),
	text(text)
{}
