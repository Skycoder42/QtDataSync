#include "stateholder.h"
using namespace QtDataSync;

StateHolder::StateHolder(QObject *parent) :
	QObject(parent)
{}

void StateHolder::initialize(Defaults *) {}

void StateHolder::finalize() {}
