#include "testobject.h"

TestObject::TestObject(QObject *parent) :
	QObject(parent),
	id(-1),
	text()
{}

TestObject::TestObject(int id, const QString &text, QObject *parent) :
	QObject(parent),
	id(id),
	text(text)
{}
