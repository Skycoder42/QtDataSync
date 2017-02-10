#include "stateholder.h"
using namespace QtDataSync;

StateHolder::StateHolder(QObject *parent) :
	QObject(parent)
{}

void StateHolder::initialize(const QString &) {}

void StateHolder::finalize(const QString &) {}
