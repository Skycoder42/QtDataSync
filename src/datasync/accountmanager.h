#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <functional>
#include <tuple>

#include <QtCore/qobject.h>
#include <QtCore/quuid.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qlist.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qjsonobject.h>

#include "qtdatasync_global.h"

class QRemoteObjectNode;
class QRemoteObjectReplica;
class AccountManagerPrivateReplica;

namespace QtDataSync {

class DeviceInfoPrivate;
class Q_DATASYNC_EXPORT DeviceInfo
{
	Q_GADGET

	Q_PROPERTY(QUuid deviceId READ deviceId WRITE setDeviceId)
	Q_PROPERTY(QString name READ name WRITE setName)
	Q_PROPERTY(QByteArray fingerprint READ fingerprint WRITE setFingerprint)

public:
	DeviceInfo();
	DeviceInfo(const QUuid &deviceId, const QString &name, const QByteArray &fingerprint);
	DeviceInfo(const std::tuple<QUuid, QString, QByteArray> &init);
	DeviceInfo(const DeviceInfo &other);
	~DeviceInfo();

	DeviceInfo &operator=(const DeviceInfo &other);

	QUuid deviceId() const;
	QString name() const;
	QByteArray fingerprint() const;

	void setDeviceId(const QUuid &deviceId);
	void setName(const QString &name);
	void setFingerprint(const QByteArray &fingerprint);

private:
	QSharedDataPointer<DeviceInfoPrivate> d;

	friend Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const DeviceInfo &deviceInfo);
	friend Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, DeviceInfo &deviceInfo);
};

class LoginRequestPrivate;
class Q_DATASYNC_EXPORT LoginRequest
{
	friend class AccountManager;

public:
	LoginRequest(LoginRequestPrivate *d_ptr);
	~LoginRequest();

	DeviceInfo device() const;
	bool handled() const;

	void accept();
	void reject();

private:
	QScopedPointer<LoginRequestPrivate> d;
};

class AccountManagerPrivateHolder;
class Q_DATASYNC_EXPORT AccountManager : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName RESET resetDeviceName NOTIFY deviceNameChanged)
	Q_PROPERTY(QByteArray deviceFingerprint READ deviceFingerprint NOTIFY deviceFingerprintChanged)

public:
	explicit AccountManager(QObject *parent = nullptr);
	explicit AccountManager(const QString &setupName, QObject *parent = nullptr);
	explicit AccountManager(QRemoteObjectNode *node, QObject *parent = nullptr);
	~AccountManager();

	QRemoteObjectReplica *replica() const;

	void resetAccount(bool keepData = true);
	void exportAccount(bool includeServer, const std::function<void(QJsonObject)> &completedFn);
	void exportAccount(bool includeServer, const std::function<void(QByteArray)> &completedFn);
	void exportAccountTrusted(bool includeServer, const QString &password, const std::function<void(QJsonObject)> &completedFn);
	void exportAccountTrusted(bool includeServer, const QString &password, const std::function<void(QByteArray)> &completedFn);
	void importAccount(const QJsonObject &importData, const std::function<void(bool,QString)> &completedFn, bool keepData = false);
	void importAccount(const QByteArray &importData, const std::function<void(bool,QString)> &completedFn, bool keepData = false);

	QString deviceName() const;
	QByteArray deviceFingerprint() const;

public Q_SLOTS:
	void listDevices();
	void removeDevice(const QUuid &deviceId);
	inline void removeDevice(const DeviceInfo &deviceInfo) {
		removeDevice(deviceInfo.deviceId());
	}

	void updateDeviceKey();
	void updateExchangeKey();

	void setDeviceName(const QString &deviceName);
	void resetDeviceName();

Q_SIGNALS:
	void accountDevices(const QList<QtDataSync::DeviceInfo> &devices);
	void loginRequested(LoginRequest * const request);

	void deviceNameChanged(const QString &deviceName);
	void deviceFingerprintChanged(const QByteArray &deviceFingerprint);

private Q_SLOTS:
	void accountExportReady(quint32 id, const QJsonObject &exportData);
	void accountImportResult(quint32 id, bool success, const QString &error);
	void loginRequestedImpl(const DeviceInfo &deviceInfo);

private:
	QScopedPointer<AccountManagerPrivateHolder> d;
};

Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const DeviceInfo &deviceInfo);
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, DeviceInfo &deviceInfo);

}

Q_DECLARE_METATYPE(QtDataSync::DeviceInfo)

#endif // ACCOUNTMANAGER_H
