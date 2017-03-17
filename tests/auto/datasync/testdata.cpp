#include "testdata.h"

TestData::TestData(int id, QString text) :
	id(id),
	text(text)
{}

bool TestData::operator ==(const TestData &other) const
{
	return id == other.id &&
			text == other.text;
}
