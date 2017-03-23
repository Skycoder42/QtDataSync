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

void RemoteConnector::resetUserId(QFutureInterface<QVariant> futureInterface, const QVariant &extraData, bool resetLocalStore)
{
	if(resetLocalStore)//resync is always done, only clear explicitly needed
		emit performLocalReset(true);//direct connected thus "inline"
	auto oldId = _defaults->settings()->value(QStringLiteral("RemoteConnector/deviceId")).toByteArray();
	_defaults->settings()->remove(QStringLiteral("RemoteConnector/deviceId"));
	resetUserData(extraData, oldId);
	reloadRemoteState();
	futureInterface.reportResult(QVariant());
	futureInterface.reportFinished();
}

Defaults *RemoteConnector::defaults() const
{
	return _defaults;
}

QByteArray RemoteConnector::getDeviceId() const
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
