#include "conflictresolver.h"
#include "conflictresolver_p.h"
using namespace QtDataSync;

ConflictResolver::ConflictResolver(QObject *parent) :
	QObject(parent),
	d(new ConflictResolverPrivate())
{}

ConflictResolver::~ConflictResolver() {}

void ConflictResolver::setDefaults(const Defaults &defaults)
{
	d->defaults = defaults;
	d->logger = d->defaults.createLogger(name(), this);
	d->settings = d->defaults.createSettings(this, QString::fromUtf8(name()));
}

QByteArray ConflictResolver::name() const
{
	return "resolver";
}

Defaults ConflictResolver::defaults() const
{
	return d->defaults;
}

Logger *ConflictResolver::logger() const
{
	return d->logger;
}

QSettings *ConflictResolver::settings() const
{
	return d->settings;
}

// ------------- Private Implementation -------------

ConflictResolverPrivate::ConflictResolverPrivate() :
	defaults(),
	logger(nullptr),
	settings(nullptr)
{}
