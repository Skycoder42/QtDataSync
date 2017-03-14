#include "defaults.h"
#include "remoteconnector.h"

#include <QtCore/QSettings>
#include <QtCore/QUuid>

using namespace QtDataSync;

RemoteConnector::RemoteConnector(QObject *parent) :
	QObject(parent),
	_defaults()
{}

void RemoteConnector::initialize(Defaults *defaults)
{
	_defaults = defaults;
}

void RemoteConnector::finalize() {}

void RemoteConnector::resetDeviceId()
{
	_defaults->settings()->remove(QStringLiteral("RemoteConnector/deviceId"));
}

Defaults *RemoteConnector::defaults() const
{
	return _defaults;
}

QByteArray RemoteConnector::loadDeviceId()
{
	static const QString key(QStringLiteral("RemoteConnector/deviceId"));
	auto settings = _defaults->settings();
	if(settings->contains(key))
		return settings->value(key).toByteArray();
	else {
		auto id = QUuid::createUuid();
		settings->setValue(key, id.toByteArray());
		return id.toByteArray();
	}
}
