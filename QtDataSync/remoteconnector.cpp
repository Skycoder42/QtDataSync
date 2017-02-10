#include "defaults.h"
#include "remoteconnector.h"

#include <QSettings>
#include <QUuid>
using namespace QtDataSync;

RemoteConnector::RemoteConnector(QObject *parent) :
	QObject(parent)
{}

void RemoteConnector::initialize(const QDir &storageDir)
{
	_storageDir = storageDir;
}

void RemoteConnector::finalize(const QDir &) {}

void RemoteConnector::resetDeviceId()
{
	auto settings = Defaults::settings(_storageDir, this);
	settings->remove(QStringLiteral("RemoteConnector/deviceId"));
	settings->deleteLater();
}

QDir RemoteConnector::storageDir() const
{
	return _storageDir;
}

QByteArray RemoteConnector::loadDeviceId()
{
	static const QString key(QStringLiteral("RemoteConnector/deviceId"));
	auto settings = Defaults::settings(_storageDir, this);
	if(settings->contains(key))
		return settings->value(key).toByteArray();
	else {
		auto id = QUuid::createUuid();
		settings->setValue(key, id.toByteArray());
		return id.toByteArray();
	}
	settings->deleteLater();
}
