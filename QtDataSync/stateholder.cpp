#include "stateholder.h"
using namespace QtDataSync;

StateHolder::StateHolder(QObject *parent) :
	QObject(parent)
{}

void StateHolder::initialize(const QDir &) {}

void StateHolder::finalize(const QDir &) {}
