#include "localstore.h"
using namespace QtDataSync;

LocalStore::LocalStore(QObject *parent) :
	QObject(parent)
{}

void LocalStore::initialize(Defaults *) {}

void LocalStore::finalize() {}
