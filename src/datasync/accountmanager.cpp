#include "accountmanager.h"
#include "defaults_p.h"

#include "rep_accountmanager_p_replica.h"

// ------------- Private classes Definition -------------

namespace QtDataSync {

class DeviceInfoPrivate : public QSharedData
{
public:
	DeviceInfoPrivate(const QUuid &deviceId = {}, const QString &name = {}, const QByteArray &fingerprint = {});
	DeviceInfoPrivate(const DeviceInfoPrivate &other);

	QUuid deviceId;
	QString name;
	QByteArray fingerprint;
};

class LoginRequestPrivate
{
public:
	LoginRequestPrivate(const DeviceInfo &deviceInfo);
	DeviceInfo device;
	bool acted;
	bool accepted;
};

class AccountManagerPrivateHolder
{
public:
	AccountManagerPrivateHolder(AccountManagerPrivateReplica *replica);

	AccountManagerPrivateReplica *replica;
	QHash<quint32, std::function<void(QJsonObject)>> exportActions;
	QHash<quint32, std::function<void(bool,QString)>> importActions;
	QHash<quint32, std::function<void(QString)>> errorActions;
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
	connect(d->replica, &AccountManagerPrivateReplica::accountExportError,
			this, &AccountManager::accountExportError);
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

void AccountManager::resetAccount(bool keepData)
{
	return d->replica->resetAccount(keepData);
}

void AccountManager::exportAccount(bool includeServer, const std::function<void(QJsonObject)> &completedFn, const std::function<void(QString)> &errorFn)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");

	quint32 id;
	do {
		id = (quint32)qrand();
	} while(d->exportActions.contains(id));

	d->exportActions.insert(id, completedFn);
	if(errorFn)
		d->errorActions.insert(id, errorFn);
	d->replica->exportAccount(id, includeServer);
}

void AccountManager::exportAccount(bool includeServer, const std::function<void(QByteArray)> &completedFn, const std::function<void(QString)> &errorFn)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");
	exportAccount(includeServer, [completedFn](QJsonObject obj) {
		completedFn(QJsonDocument(obj).toJson(QJsonDocument::Compact));
	}, errorFn);
}

void AccountManager::exportAccountTrusted(bool includeServer, const QString &password, const std::function<void (QJsonObject)> &completedFn, const std::function<void(QString)> &errorFn)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");

	quint32 id;
	do {
		id = (quint32)qrand();
	} while(d->exportActions.contains(id));

	d->exportActions.insert(id, completedFn);
	if(errorFn)
		d->errorActions.insert(id, errorFn);
	d->replica->exportAccountTrusted(id, includeServer, password);
}

void AccountManager::exportAccountTrusted(bool includeServer, const QString &password, const std::function<void(QByteArray)> &completedFn, const std::function<void(QString)> &errorFn)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");
	exportAccountTrusted(includeServer, password, [completedFn](QJsonObject obj) {
		completedFn(QJsonDocument(obj).toJson(QJsonDocument::Compact));
	}, errorFn);
}

void AccountManager::importAccount(const QJsonObject &importData, const std::function<void (bool, QString)> &completedFn, bool keepData)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");

	quint32 id;
	do {
		id = (quint32)qrand();
	} while(d->importActions.contains(id));

	d->importActions.insert(id, completedFn);
	d->replica->importAccount(id, importData, keepData);
}

void AccountManager::importAccount(const QByteArray &importData, const std::function<void(bool,QString)> &completedFn, bool keepData)
{
	importAccount(QJsonDocument::fromJson(importData).object(), completedFn, keepData);
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

void AccountManager::removeDevice(const QUuid &deviceId)
{
	d->replica->removeDevice(deviceId);
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

void AccountManager::resetDeviceName()
{
	d->replica->setDeviceName(QString());
}

void AccountManager::accountExportReady(quint32 id, const QJsonObject &exportData)
{
	auto completeFn = d->exportActions.take(id);
	if(completeFn)
		completeFn(exportData);
	d->errorActions.remove(id);
}

void AccountManager::accountExportError(quint32 id, const QString &errorString)
{
	auto errorFn = d->errorActions.take(id);
	if(errorFn)
		errorFn(errorString);
	d->exportActions.remove(id);
}

void AccountManager::accountImportResult(quint32 id, bool success, const QString &error)
{
	auto completeFn = d->importActions.take(id);
	if(completeFn)
		completeFn(success, error);
}

void AccountManager::loginRequestedImpl(const DeviceInfo &deviceInfo)
{
	LoginRequest request(new LoginRequestPrivate(deviceInfo));
	emit loginRequested(&request);
	if(request.d->acted) //if any slot connected actually did something
		d->replica->replyToLogin(deviceInfo.deviceId(), request.d->accepted);
}



AccountManagerPrivateHolder::AccountManagerPrivateHolder(AccountManagerPrivateReplica *replica) :
	replica(replica),
	exportActions(),
	importActions(),
	errorActions()
{}

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



LoginRequestPrivate::LoginRequestPrivate(const DeviceInfo &deviceInfo) :
	device(deviceInfo),
	acted(false),
	accepted(false)
{}

// ------------- DeviceInfo Implementation -------------

DeviceInfo::DeviceInfo() :
	d(new DeviceInfoPrivate())
{}

DeviceInfo::DeviceInfo(const QUuid &deviceId, const QString &name, const QByteArray &fingerprint) :
	d(new DeviceInfoPrivate(deviceId, name, fingerprint))
{}

DeviceInfo::DeviceInfo(const std::tuple<QUuid, QString, QByteArray> &init) :
	DeviceInfo(std::get<0>(init), std::get<1>(init), std::get<2>(init))
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

QUuid DeviceInfo::deviceId() const
{
	return d->deviceId;
}

QString DeviceInfo::name() const
{
	return d->name;
}

QByteArray DeviceInfo::fingerprint() const
{
	return d->fingerprint;
}

void DeviceInfo::setDeviceId(const QUuid &deviceId)
{
	d->deviceId = deviceId;
}

void DeviceInfo::setName(const QString &name)
{
	d->name = name;
}

void DeviceInfo::setFingerprint(const QByteArray &fingerprint)
{
	d->fingerprint = fingerprint;
}



DeviceInfoPrivate::DeviceInfoPrivate(const QUuid &deviceId, const QString &name, const QByteArray &fingerprint) :
	QSharedData(),
	deviceId(deviceId),
	name(name),
	fingerprint(fingerprint)
{}

DeviceInfoPrivate::DeviceInfoPrivate(const DeviceInfoPrivate &other) :
	QSharedData(other),
	deviceId(other.deviceId),
	name(other.name),
	fingerprint(other.fingerprint)
{}



QDataStream &QtDataSync::operator<<(QDataStream &stream, const DeviceInfo &deviceInfo)
{
	stream << deviceInfo.d->deviceId
		   << deviceInfo.d->name
		   << deviceInfo.d->fingerprint;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, DeviceInfo &deviceInfo)
{
	stream.startTransaction();
	stream >> deviceInfo.d->deviceId
		   >> deviceInfo.d->name
		   >> deviceInfo.d->fingerprint;
	stream.commitTransaction();
	return stream;
}
