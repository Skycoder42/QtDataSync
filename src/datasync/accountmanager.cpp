#include "accountmanager.h"
#include "defaults_p.h"

#include "rep_accountmanager_p_replica.h"

// ------------- Private classes Definition -------------

namespace QtDataSync {

class DeviceInfoPrivate : public QSharedData
{
public:
	DeviceInfoPrivate(const QString &name = {}, const QByteArray &fingerprint = {});
	DeviceInfoPrivate(const DeviceInfoPrivate &other);

	QString name;
	QByteArray fingerprint;
};

class LoginRequestPrivate
{
public:
	LoginRequestPrivate(const QString &name, const QByteArray &fingerprint);
	DeviceInfo device;
	bool acted;
	bool accepted;
};

class AccountManagerPrivateHolder
{
public:
	AccountManagerPrivateHolder(AccountManagerPrivateReplica *replica);

	AccountManagerPrivateReplica *replica;
	QHash<quint32, std::function<void(QByteArray)>> exportActions;
	QHash<quint32, std::function<void(bool,QString)>> importActions;
};

}

using namespace QtDataSync;

// ------------- AccountManager Implementation -------------

AccountManager::AccountManager(QObject *parent) :
	AccountManager(DefaultSetup, parent)
{}

AccountManager::AccountManager(const QString &setupName, QObject *parent) :
	AccountManager(Defaults(DefaultsPrivate::obtainDefaults(setupName)).remoteNode(), parent)
{}

AccountManager::AccountManager(QRemoteObjectNode *node, QObject *parent) :
	QObject(parent),
	d(new AccountManagerPrivateHolder(node->acquire<AccountManagerPrivateReplica>()))
{
	d->replica->setParent(this);
	connect(d->replica, &AccountManagerPrivateReplica::deviceNameChanged,
			this, &AccountManager::deviceNameChanged);
	connect(d->replica, &AccountManagerPrivateReplica::deviceFingerprintChanged,
			this, &AccountManager::deviceFingerprintChanged);
	connect(d->replica, &AccountManagerPrivateReplica::accountDevices,
			this, &AccountManager::accountDevices);
	connect(d->replica, &AccountManagerPrivateReplica::accountExportReady,
			this, &AccountManager::accountExportReady);
	connect(d->replica, &AccountManagerPrivateReplica::accountImportResult,
			this, &AccountManager::accountImportResult);
	connect(d->replica, &AccountManagerPrivateReplica::loginRequested,
			this, &AccountManager::loginRequestedImpl);
}

AccountManager::~AccountManager() {}

QRemoteObjectReplica *AccountManager::replica() const
{
	return d->replica;
}

void AccountManager::exportAccount(bool includeServer, const std::function<void(QByteArray)> &completedFn)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");

	quint32 id;
	do {
		id = (quint32)qrand();
	} while(d->exportActions.contains(id));

	d->exportActions.insert(id, completedFn);
	d->replica->exportAccount(id, includeServer);
}

void AccountManager::exportAccountTrusted(bool includeServer, const QString &password, const std::function<void(QByteArray)> &completedFn)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");

	quint32 id;
	do {
		id = (quint32)qrand();
	} while(d->exportActions.contains(id));

	d->exportActions.insert(id, completedFn);
	d->replica->exportAccountTrusted(id, includeServer, password);
}

void AccountManager::importAccount(const QByteArray &importData, const std::function<void(bool,QString)> &completedFn)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");

	quint32 id;
	do {
		id = (quint32)qrand();
	} while(d->importActions.contains(id));

	d->importActions.insert(id, completedFn);
	d->replica->importAccount(id, importData);
}

QString AccountManager::deviceName() const
{
	return d->replica->deviceName();
}

QByteArray AccountManager::deviceFingerprint() const
{
	return d->replica->deviceFingerprint();
}

void AccountManager::listDevices()
{
	d->replica->listDevices();
}

void AccountManager::removeDevice(const QByteArray &fingerprint)
{
	d->replica->removeDevice(fingerprint);
}

void AccountManager::updateDeviceKey()
{
	d->replica->updateDeviceKey();
}

void AccountManager::updateExchangeKey()
{
	d->replica->updateExchangeKey();
}

void AccountManager::setDeviceName(const QString &deviceName)
{
	d->replica->setDeviceName(deviceName);
}

void AccountManager::accountExportReady(quint32 id, const QByteArray &exportData)
{
	auto completeFn = d->exportActions.take(id);
	if(completeFn)
		completeFn(exportData);
}

void AccountManager::accountImportResult(quint32 id, bool success, const QString &error)
{
	auto completeFn = d->importActions.take(id);
	if(completeFn)
		completeFn(success, error);
}

void AccountManager::loginRequestedImpl(const QString &name, const QByteArray &fingerprint)
{
	LoginRequest request(new LoginRequestPrivate(name, fingerprint));
	emit loginRequested(&request);
	if(request.d->acted) //if any slot connected actually did something
		d->replica->replyToLogin(fingerprint, request.d->accepted);
}



AccountManagerPrivateHolder::AccountManagerPrivateHolder(AccountManagerPrivateReplica *replica) :
	replica(replica),
	exportActions(),
	importActions()
{}



QDataStream &QtDataSync::operator<<(QDataStream &stream, const DeviceInfo &deviceInfo)
{
	stream << deviceInfo.d->name
		   << deviceInfo.d->fingerprint;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, DeviceInfo &deviceInfo)
{
	stream.startTransaction();
	stream >> deviceInfo.d->name
		   >> deviceInfo.d->fingerprint;
	stream.commitTransaction();
	return stream;
}

// ------------- LoginRequest Implementation -------------

LoginRequest::LoginRequest(LoginRequestPrivate *d_ptr) :
	d(d_ptr)
{}

LoginRequest::~LoginRequest() {}

DeviceInfo LoginRequest::device() const
{
	return d->device;
}

bool LoginRequest::handled() const
{
	return d->acted;
}

void LoginRequest::accept()
{
	d->acted = true;
	d->accepted = true;
}

void LoginRequest::reject()
{
	d->acted = true;
	d->accepted = false;
}



LoginRequestPrivate::LoginRequestPrivate(const QString &name, const QByteArray &fingerprint) :
	device(name, fingerprint),
	acted(false),
	accepted(false)
{}

// ------------- DeviceInfo Implementation -------------

DeviceInfo::DeviceInfo() :
	d(new DeviceInfoPrivate())
{}

DeviceInfo::DeviceInfo(const QString &name, const QByteArray &fingerprint) :
	d(new DeviceInfoPrivate(name, fingerprint))
{}

DeviceInfo::DeviceInfo(const DeviceInfo &other) :
	d(other.d)
{}

DeviceInfo::~DeviceInfo() {}

DeviceInfo &DeviceInfo::operator=(const DeviceInfo &other)
{
	d = other.d;
	return (*this);
}

QString DeviceInfo::name() const
{
	return d->name;
}

QByteArray DeviceInfo::fingerprint() const
{
	return d->fingerprint;
}

void DeviceInfo::setName(const QString &name)
{
	d->name = name;
}

void DeviceInfo::setFingerprint(const QByteArray &fingerprint)
{
	d->fingerprint = fingerprint;
}



DeviceInfoPrivate::DeviceInfoPrivate(const QString &name, const QByteArray &fingerprint) :
	QSharedData(),
	name(name),
	fingerprint(fingerprint)
{}

DeviceInfoPrivate::DeviceInfoPrivate(const DeviceInfoPrivate &other) :
	QSharedData(other),
	name(other.name),
	fingerprint(other.fingerprint)
{}
