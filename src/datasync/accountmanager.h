#ifndef QTDATASYNC_ACCOUNTMANAGER_H
#define QTDATASYNC_ACCOUNTMANAGER_H

#include <functional>

#include <QtCore/qobject.h>
#include <QtCore/quuid.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qlist.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qjsonobject.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/remoteconfig.h"

class QRemoteObjectNode;
class QRemoteObjectReplica;

namespace QtDataSync {

class DeviceInfoPrivate;
//! Information about a device that is part of an account or wants to be added
class Q_DATASYNC_EXPORT DeviceInfo
{
	Q_GADGET

	//! The unique id that device is identified by
	Q_PROPERTY(QUuid deviceId READ deviceId WRITE setDeviceId)
	//! The display name of the device
	Q_PROPERTY(QString name READ name WRITE setName)
	//! The fingerprint of the devices cryptographic keys
	Q_PROPERTY(QByteArray fingerprint READ fingerprint WRITE setFingerprint)

public:
	//! Default constructor
	DeviceInfo();
	//! Constructor from data
	DeviceInfo(const QUuid &deviceId, const QString &name, const QByteArray &fingerprint);
	//! Copy constructor
	DeviceInfo(const DeviceInfo &other);
	~DeviceInfo();

	//! Assignment operator
	DeviceInfo &operator=(const DeviceInfo &other);

	//! @readAcFn{DeviceInfo::deviceId}
	QUuid deviceId() const;
	//! @readAcFn{DeviceInfo::name}
	QString name() const;
	//! @readAcFn{DeviceInfo::fingerprint}
	QByteArray fingerprint() const;

	//! @writeAcFn{DeviceInfo::deviceId}
	void setDeviceId(const QUuid &deviceId);
	//! @writeAcFn{DeviceInfo::name}
	void setName(const QString &name);
	//! @writeAcFn{DeviceInfo::fingerprint}
	void setFingerprint(const QByteArray &fingerprint);

private:
	QSharedDataPointer<DeviceInfoPrivate> d;

	friend Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const DeviceInfo &deviceInfo);
	friend Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, DeviceInfo &deviceInfo);
};

class LoginRequestPrivate;
//! A login request done by another device to this device
class Q_DATASYNC_EXPORT LoginRequest
{
	Q_GADGET
	friend class AccountManager;

	//! The device that requested to be added to the account
	Q_PROPERTY(QtDataSync::DeviceInfo device READ device CONSTANT)
	//! Indicates whether this request has already been handled
	Q_PROPERTY(bool handled READ handled CONSTANT)

public:
	//! @private
	LoginRequest(LoginRequestPrivate *d_ptr = nullptr);
	~LoginRequest();

	//! @readAcFn{LoginRequest::device}
	DeviceInfo device() const;
	//! @readAcFn{LoginRequest::handled}
	bool handled() const;

	//! Accepts the login request if it has not been handled yet. Marks is as handled
	Q_INVOKABLE void accept();
	//! Rejects the login request if it has not been handled yet. Marks is as handled
	Q_INVOKABLE void reject();

private:
	QSharedPointer<LoginRequestPrivate> d;
};

class AccountManagerPrivateHolder;
//! Manages devices that belong to the users account
class Q_DATASYNC_EXPORT AccountManager : public QObject
{
	Q_OBJECT

	//! The display name of the device
	Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName RESET resetDeviceName NOTIFY deviceNameChanged)
	//! The fingerprint of the devices cryptographic keys
	Q_PROPERTY(QByteArray deviceFingerprint READ deviceFingerprint NOTIFY deviceFingerprintChanged)
	//! The last internal error that occured
	Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
	//! Default constructor, uses the default setup
	explicit AccountManager(QObject *parent = nullptr);
	//! Constructor with an explicit setup
	explicit AccountManager(const QString &setupName, QObject *parent = nullptr);
	//! Constructor with a remote object node to connect to
	explicit AccountManager(QRemoteObjectNode *node, QObject *parent = nullptr);
	~AccountManager();

	//! The internal replica used to connect to the engine
	Q_INVOKABLE QRemoteObjectReplica *replica() const;

	//! Checks if the given data is trusted or not
	static bool isTrustedImport(const QJsonObject &importData);
	//! @copydoc isTrustedImport(const QJsonObject &)
	static bool isTrustedImport(const QByteArray &importData);

	//! Export the current account data as untrusted export
	void exportAccount(bool includeServer, const std::function<void(QJsonObject)> &completedFn, const std::function<void(QString)> &errorFn = {});
	//! @copydoc exportAccount(bool, const std::function<void(QJsonObject)> &, const std::function<void(QString)> &)
	void exportAccount(bool includeServer, const std::function<void(QByteArray)> &completedFn, const std::function<void(QString)> &errorFn = {});
	//! Export the current account data as trusted export
	void exportAccountTrusted(bool includeServer, const QString &password, const std::function<void(QJsonObject)> &completedFn, const std::function<void(QString)> &errorFn = {});
	//! @copydoc exportAccountTrusted(bool, const QString &, const std::function<void(QJsonObject)> &, const std::function<void(QString)> &)
	void exportAccountTrusted(bool includeServer, const QString &password, const std::function<void(QByteArray)> &completedFn, const std::function<void(QString)> &errorFn = {});
	//! Import an account from the given untrusted export data
	void importAccount(const QJsonObject &importData, const std::function<void(bool,QString)> &completedFn, bool keepData = false);
	//! @copydoc importAccount(const QJsonObject &, const std::function<void(bool,QString)> &, bool)
	void importAccount(const QByteArray &importData, const std::function<void(bool,QString)> &completedFn, bool keepData = false);
	//! Import an account from the given trusted export data
	void importAccountTrusted(const QJsonObject &importData, const QString &password, const std::function<void(bool,QString)> &completedFn, bool keepData = false);
	//! @copydoc importAccountTrusted(const QJsonObject &, const QString &, const std::function<void(bool,QString)> &, bool)
	void importAccountTrusted(const QByteArray &importData, const QString &password, const std::function<void(bool,QString)> &completedFn, bool keepData = false);

	//! @readAcFn{AccountManager::deviceName}
	QString deviceName() const;
	//! @readAcFn{AccountManager::deviceFingerprint}
	QByteArray deviceFingerprint() const;
	//! @readAcFn{AccountManager::lastError}
	QString lastError() const;

public Q_SLOTS:
	//! Sends a request to the server to list all devices that belong to the current account
	void listDevices();
	//! Remove the device with the given id from the current account
	void removeDevice(const QUuid &deviceInfo);
	//! Remove the given device from the current account
	inline void removeDevice(const QtDataSync::DeviceInfo &deviceInfo) {
		removeDevice(deviceInfo.deviceId());
	}
	//! Removes this device from the current account and then creates a new one
	void resetAccount(bool keepData = true);
	//! Resets the account and connects to the new remote to create a new one
	void changeRemote(const RemoteConfig &config, bool keepData = true);

	//! Generate a new secret exchange key used to encrypt data
	void updateExchangeKey();

	//! @writeAcFn{AccountManager::deviceName}
	void setDeviceName(const QString &deviceName);
	//! @resetAcFn{AccountManager::deviceName}
	void resetDeviceName();

Q_SIGNALS:
	//! Is emitted when the accounts devices have been sent by the server
	void accountDevices(const QList<QtDataSync::DeviceInfo> &devices, QPrivateSignal);
	//! Is emitted when another device requests permission to be added to the current account
	void loginRequested(QtDataSync::LoginRequest request, QPrivateSignal);
	//! Is emitted when the partner accept the account import
	void importAccepted(QPrivateSignal);
	//! Is emitted when a device has been granted access to the current account
	void accountAccessGranted(const QUuid &deviceId, QPrivateSignal);

	//! @notifyAcFn{AccountManager::deviceName}
	void deviceNameChanged(const QString &deviceName, QPrivateSignal);
	//! @notifyAcFn{AccountManager::deviceFingerprint}
	void deviceFingerprintChanged(const QByteArray &deviceFingerprint, QPrivateSignal);
	//! @notifyAcFn{AccountManager::lastError}
	void lastErrorChanged(const QString &lastError, QPrivateSignal);

protected:
	//! @private
	AccountManager(QObject *parent, void *);
	//! @private
	void initReplica(const QString &setupName);
	//! @private
	void initReplica(QRemoteObjectNode *node);

private Q_SLOTS:
	void accountExportReady(const QUuid &id, const QJsonObject &exportData);
	void accountExportError(const QUuid &id, const QString &errorString);
	void accountImportResult(bool success, const QString &error);
	void loginRequestedImpl(const DeviceInfo &deviceInfo);

private:
	QScopedPointer<AccountManagerPrivateHolder> d;
};

//! Stream operator to stream into a QDataStream
Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const DeviceInfo &deviceInfo);
//! Stream operator to stream out of a QDataStream
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, DeviceInfo &deviceInfo);

//helper class until QTBUG-65557 is fixed (not exported (!), internal only)
//! @private
class JsonObject : public QJsonObject {
public:
	inline JsonObject(const QJsonObject &other = {}) :
		QJsonObject(other)
	{}
};

//! @private
QDataStream &operator<<(QDataStream &out, const JsonObject &data);
//! @private
QDataStream &operator>>(QDataStream &in, JsonObject &data);

}

Q_DECLARE_METATYPE(QtDataSync::JsonObject)
Q_DECLARE_METATYPE(QtDataSync::DeviceInfo)
Q_DECLARE_TYPEINFO(QtDataSync::DeviceInfo, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(QtDataSync::LoginRequest)

#endif // QTDATASYNC_ACCOUNTMANAGER_H
