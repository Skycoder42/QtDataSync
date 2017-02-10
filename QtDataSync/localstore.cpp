#include "localstore.h"
using namespace QtDataSync;

LocalStore::LocalStore(QObject *parent) :
	QObject(parent)
{}

void LocalStore::initialize(const QDir &) {}

void LocalStore::finalize(const QDir &) {}
