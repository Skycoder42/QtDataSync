#include "testobject.h"

TestObject::TestObject(QObject *parent) :
	QObject(parent),
	id(0),
	text()
{}

bool TestObject::equals(const TestObject *other) const
{
	return id == other->id &&
			text == other->text;
}
