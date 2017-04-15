#include "defaults.h"
#include "remoteconnector.h"

#include <QtCore/QSettings>
#include <QtCore/QUuid>

namespace QtDataSync {
class RemoteConnectorPrivate {
public:
	RemoteConnectorPrivate();

	Defaults *defaults;
	Encryptor *cryptor;
};
}

using namespace QtDataSync;

RemoteConnector::RemoteConnector(QObject *parent) :
	QObject(parent),
	d(new RemoteConnectorPrivate())
{}

RemoteConnector::~RemoteConnector() {}

void RemoteConnector::initialize(Defaults *defaults, Encryptor *cryptor)
{
	d->defaults = defaults;
	d->cryptor = cryptor;
}

void RemoteConnector::finalize() {}

Encryptor *RemoteConnector::cryptor() const
{
	return d->cryptor;
}

void RemoteConnector::resetUserId(QFutureInterface<QVariant> futureInterface, const QVariant &extraData, bool resetLocalStore)
{
	if(resetLocalStore)//resync is always done, only clear explicitly needed
		emit performLocalReset(true);//direct connected thus "inline"
	auto oldId = d->defaults->settings()->value(QStringLiteral("RemoteConnector/deviceId")).toByteArray();
	d->defaults->settings()->remove(QStringLiteral("RemoteConnector/deviceId"));
	resetUserData(extraData, oldId);
	reloadRemoteState();
	futureInterface.reportResult(QVariant());
	futureInterface.reportFinished();
}

Defaults *RemoteConnector::defaults() const
{
	return d->defaults;
}

QByteArray RemoteConnector::getDeviceId() const
{
	static const QString key(QStringLiteral("RemoteConnector/deviceId"));
	auto settings = d->defaults->settings();
	if(settings->contains(key))
		return settings->value(key).toByteArray();
	else {
		auto id = QUuid::createUuid();
		settings->setValue(key, id.toByteArray());
		return id.toByteArray();
	}
}


RemoteConnectorPrivate::RemoteConnectorPrivate() :
	defaults(nullptr),
	cryptor(nullptr)
{}
