#include "remoteconnector.h"

#include <QSettings>
#include <QUuid>
using namespace QtDataSync;

RemoteConnector::RemoteConnector(QObject *parent) :
	QObject(parent)
{}

void RemoteConnector::initialize() {}

void RemoteConnector::finalize() {}

void RemoteConnector::resetDeviceId()
{
	QSettings().remove(QStringLiteral("__QtDataSync/RemoteConnector/deviceId"));
}

QByteArray RemoteConnector::loadDeviceId()
{
	static const QString key(QStringLiteral("__QtDataSync/RemoteConnector/deviceId"));
	QSettings settings;
	if(settings.contains(key))
		return settings.value(key).toByteArray();
	else {
		auto id = QUuid::createUuid();
		settings.setValue(key, id.toByteArray());
		return id.toByteArray();
	}
}
