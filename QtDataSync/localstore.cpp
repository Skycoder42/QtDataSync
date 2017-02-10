#include "localstore.h"
using namespace QtDataSync;

LocalStore::LocalStore(QObject *parent) :
	QObject(parent)
{}

void LocalStore::initialize(const QString &) {}

void LocalStore::finalize(const QString &) {}
