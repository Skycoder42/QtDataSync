#include "conflictresolver.h"
#include "conflictresolver_p.h"
#include "defaults_p.h"
using namespace QtDataSync;

ConflictResolver::ConflictResolver(QObject *parent) :
	QObject{parent},
	d{new ConflictResolverPrivate()}
{}

ConflictResolver::~ConflictResolver() = default;

void ConflictResolver::setDefaults(const Defaults &defaults)
{
	d->defaultsName = defaults.setupName();
	d->logger = defaults.createLogger(name(), this);
	d->settings = defaults.createSettings(this, QString::fromUtf8(name()));
}

QByteArray ConflictResolver::name() const
{
	return "resolver";
}

Defaults ConflictResolver::defaults() const
{
	return DefaultsPrivate::obtainDefaults(d->defaultsName);
}

Logger *ConflictResolver::logger() const
{
	return d->logger;
}

QSettings *ConflictResolver::settings() const
{
	return d->settings;
}
