#include "accountmanager.h"
#include "defaults_p.h"
#include "message_p.h"

#include "rep_accountmanager_p_replica.h"
#include "signal_private_connect_p.h"

using std::function;

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
	LoginRequestPrivate(const DeviceInfo &deviceInfo, AccountManagerPrivateReplica *replica);
	DeviceInfo device;
	bool acted;

	QPointer<AccountManagerPrivateReplica> replica;
};

class AccountManagerPrivateHolder
{
public:
	AccountManagerPrivateHolder();

	AccountManagerPrivateReplica *replica;
	QHash<QUuid, function<void(QJsonObject)>> exportActions;
	QHash<QUuid, function<void(QString)>> errorActions;
	function<void(bool,QString)> importAction;
};

}

using namespace QtDataSync;

// ------------- AccountManager Implementation -------------

AccountManager::AccountManager(QObject *parent) :
	AccountManager(DefaultSetup, parent)
{}

AccountManager::AccountManager(const QString &setupName, QObject *parent) :
	QObject(parent),
	d(new AccountManagerPrivateHolder())
{
	initReplica(setupName);
}

AccountManager::AccountManager(QRemoteObjectNode *node, QObject *parent) :
	QObject(parent),
	d(new AccountManagerPrivateHolder())
{
	initReplica(node);
}

AccountManager::AccountManager(QObject *parent, void *) :
	QObject(parent),
	d(new AccountManagerPrivateHolder())
{} //No init

void AccountManager::initReplica(const QString &setupName)
{
	initReplica(Defaults(DefaultsPrivate::obtainDefaults(setupName)).remoteNode());
}

void AccountManager::initReplica(QRemoteObjectNode *node)
{
	d->replica = node->acquire<AccountManagerPrivateReplica>();
	d->replica->setParent(this);

	connect(d->replica, &AccountManagerPrivateReplica::deviceNameChanged,
			this, PSIG(&AccountManager::deviceNameChanged));
	connect(d->replica, &AccountManagerPrivateReplica::deviceFingerprintChanged,
			this, PSIG(&AccountManager::deviceFingerprintChanged));
	connect(d->replica, &AccountManagerPrivateReplica::lastErrorChanged,
			this, PSIG(&AccountManager::lastErrorChanged));
	connect(d->replica, &AccountManagerPrivateReplica::accountDevices,
			this, PSIG(&AccountManager::accountDevices));
	connect(d->replica, &AccountManagerPrivateReplica::accountExportReady,
			this, &AccountManager::accountExportReady);
	connect(d->replica, &AccountManagerPrivateReplica::accountExportError,
			this, &AccountManager::accountExportError);
	connect(d->replica, &AccountManagerPrivateReplica::accountImportResult,
			this, &AccountManager::accountImportResult);
	connect(d->replica, &AccountManagerPrivateReplica::loginRequested,
			this, &AccountManager::loginRequestedImpl);
	connect(d->replica, &AccountManagerPrivateReplica::importCompleted,
			this, PSIG(&AccountManager::importAccepted));
	connect(d->replica, &AccountManagerPrivateReplica::accountAccessGranted,
			this, PSIG(&AccountManager::accountAccessGranted));
}

AccountManager::~AccountManager() {}

QRemoteObjectReplica *AccountManager::replica() const
{
	return d->replica;
}

bool AccountManager::isTrustedImport(const QJsonObject &importData)
{
	return importData[QStringLiteral("trusted")].toBool();
}

bool AccountManager::isTrustedImport(const QByteArray &importData)
{
	return isTrustedImport(QJsonDocument::fromJson(importData).object());
}

void AccountManager::resetAccount(bool keepData)
{
	d->replica->resetAccount(keepData);
}

void AccountManager::changeRemote(const RemoteConfig &config, bool keepData)
{
	d->replica->changeRemote(config, keepData);
}

void AccountManager::exportAccount(bool includeServer, const function<void(QJsonObject)> &completedFn, const function<void(QString)> &errorFn)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");
	auto id = QUuid::createUuid();
	d->exportActions.insert(id, completedFn);
	if(errorFn)
		d->errorActions.insert(id, errorFn);
	d->replica->exportAccount(id, includeServer);
}

void AccountManager::exportAccount(bool includeServer, const function<void(QByteArray)> &completedFn, const function<void(QString)> &errorFn)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");
	exportAccount(includeServer, [completedFn](QJsonObject obj) {
		completedFn(QJsonDocument(obj).toJson(QJsonDocument::Compact));
	}, errorFn);
}

void AccountManager::exportAccountTrusted(bool includeServer, const QString &password, const function<void (QJsonObject)> &completedFn, const function<void(QString)> &errorFn)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");

	if(password.isEmpty()) {
		if(errorFn)
			errorFn(tr("Password must not be empty."));
		return;
	}
	auto id = QUuid::createUuid();
	d->exportActions.insert(id, completedFn);
	if(errorFn)
		d->errorActions.insert(id, errorFn);
	d->replica->exportAccountTrusted(id, includeServer, password);
}

void AccountManager::exportAccountTrusted(bool includeServer, const QString &password, const function<void(QByteArray)> &completedFn, const function<void(QString)> &errorFn)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");
	exportAccountTrusted(includeServer, password, [completedFn](QJsonObject obj) {
		completedFn(QJsonDocument(obj).toJson(QJsonDocument::Compact));
	}, errorFn);
}

void AccountManager::importAccount(const QJsonObject &importData, const function<void (bool, QString)> &completedFn, bool keepData)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");

	if(d->importAction)
		completedFn(false, tr("Already importing. Only one import at a time is possible."));
	else {
		d->importAction = completedFn;
		d->replica->importAccount(importData, keepData);
	}
}

void AccountManager::importAccount(const QByteArray &importData, const function<void(bool,QString)> &completedFn, bool keepData)
{
	importAccount(QJsonDocument::fromJson(importData).object(), completedFn, keepData);
}

void AccountManager::importAccountTrusted(const QJsonObject &importData, const QString &password, const function<void (bool, QString)> &completedFn, bool keepData)
{
	Q_ASSERT_X(completedFn, Q_FUNC_INFO, "completedFn must be a valid function");

	if(d->importAction)
		completedFn(false, tr("Already importing. Only one import at a time is possible."));
	else {
		d->importAction = completedFn;
		d->replica->importAccountTrusted(importData, password, keepData);
	}
}

void AccountManager::importAccountTrusted(const QByteArray &importData, const QString &password, const function<void (bool, QString)> &completedFn, bool keepData)
{
	importAccountTrusted(QJsonDocument::fromJson(importData).object(), password, completedFn, keepData);
}

QString AccountManager::deviceName() const
{
	return d->replica->deviceName();
}

QByteArray AccountManager::deviceFingerprint() const
{
	return d->replica->deviceFingerprint();
}

QString AccountManager::lastError() const
{
	return d->replica->lastError();
}

void AccountManager::listDevices()
{
	d->replica->listDevices();
}

void AccountManager::removeDevice(const QUuid &deviceInfo)
{
	d->replica->removeDevice(deviceInfo);
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

void AccountManager::accountExportReady(const QUuid &id, const QJsonObject &exportData)
{
	auto completeFn = d->exportActions.take(id);
	if(completeFn)
		completeFn(exportData);
	d->errorActions.remove(id);
}

void AccountManager::accountExportError(const QUuid &id, const QString &errorString)
{
	auto errorFn = d->errorActions.take(id);
	if(errorFn)
		errorFn(errorString);
	d->exportActions.remove(id);
}

void AccountManager::accountImportResult(bool success, const QString &error)
{
	if(d->importAction) {
		d->importAction(success, error);
		d->importAction = {};
	}
}

void AccountManager::loginRequestedImpl(const DeviceInfo &deviceInfo)
{
	emit loginRequested(new LoginRequestPrivate(deviceInfo, d->replica), {});
}



AccountManagerPrivateHolder::AccountManagerPrivateHolder() :
	replica(nullptr),
	exportActions(),
	errorActions(),
	importAction()
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
	if(d->acted)
		return;

	d->acted = true;
	if(d->replica)
		d->replica->replyToLogin(d->device.deviceId(), true);
	else
		QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, "qtdatasync.LoginRequest").debug() << "skipping accept of already handled login reply";
}

void LoginRequest::reject()
{
	if(d->acted)
		return;

	d->acted = true;
	if(d->replica)
		d->replica->replyToLogin(d->device.deviceId(), false);
	else
		QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, "qtdatasync.LoginRequest").debug() << "skipping reject of already handled login reply";
}



LoginRequestPrivate::LoginRequestPrivate(const DeviceInfo &deviceInfo, AccountManagerPrivateReplica *replica) :
	device(deviceInfo),
	acted(false),
	replica(replica)
{}

// ------------- DeviceInfo Implementation -------------

DeviceInfo::DeviceInfo() :
	d(new DeviceInfoPrivate())
{}

DeviceInfo::DeviceInfo(const QUuid &deviceId, const QString &name, const QByteArray &fingerprint) :
	d(new DeviceInfoPrivate(deviceId, name, fingerprint))
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
		   << static_cast<Utf8String>(deviceInfo.d->name)
		   << deviceInfo.d->fingerprint;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, DeviceInfo &deviceInfo)
{
	Utf8String name;
	stream >> deviceInfo.d->deviceId
		   >> name
		   >> deviceInfo.d->fingerprint;
	deviceInfo.d->name = name;
	return stream;
}



QDataStream &QtDataSync::operator<<(QDataStream &stream, const JsonObject &data)
{
	stream << QJsonDocument(data).toBinaryData();
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, JsonObject &data)
{
	QByteArray bin;
	stream.startTransaction();
	stream >> bin;
	auto doc = QJsonDocument::fromBinaryData(bin);
	if(doc.isObject()) {
		data = doc.object();
		stream.commitTransaction();
	} else
		stream.abortTransaction();
	return stream;
}

