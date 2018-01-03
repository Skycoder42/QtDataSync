#ifndef REMOTECONNECTOR_P_H
#define REMOTECONNECTOR_P_H

#include <chrono>

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QTimer>

#include <QtWebSockets/QWebSocket>

#include "qtdatasync_global.h"
#include "controller_p.h"
#include "defaults.h"
#include "cryptocontroller_p.h"
#include "accountmanager.h"

#include "errormessage_p.h"
#include "identifymessage_p.h"
#include "accountmessage_p.h"
#include "welcomemessage_p.h"
#include "changemessage_p.h"
#include "changedmessage_p.h"
#include "devicesmessage_p.h"
#include "removemessage_p.h"

class ConnectorStateMachine;

namespace QtDataSync {

class Q_DATASYNC_EXPORT ExportData
{
public:
	ExportData();

	QByteArray pNonce;
	QUuid partnerId;
	bool trusted;
	QByteArray signature;
	QSharedPointer<RemoteConfig> config;

	QByteArray signData() const;
};

class Q_DATASYNC_EXPORT RemoteConnector : public Controller
{
	Q_OBJECT

	Q_PROPERTY(bool syncEnabled READ isSyncEnabled WRITE setSyncEnabled NOTIFY syncEnabledChanged)
	Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName RESET resetDeviceName NOTIFY deviceNameChanged)

public:
	static const QString keyRemoteEnabled;
	static const QString keyRemoteUrl;
	static const QString keyAccessKey;
	static const QString keyHeaders;
	static const QString keyKeepaliveTimeout;
	static const QString keyDeviceId;
	static const QString keyDeviceName;

	enum RemoteEvent {
		RemoteDisconnected,
		RemoteConnecting,
		RemoteReady,
		RemoteReadyWithChanges
	};
	Q_ENUM(RemoteEvent)

	explicit RemoteConnector(const Defaults &defaults, QObject *parent = nullptr);

	CryptoController *cryptoController() const;

	void initialize(const QVariantHash &params) final;
	void finalize() final;

	ExportData exportAccount(bool trusted, bool includeServer);

	bool isSyncEnabled() const;
	QString deviceName() const;

public Q_SLOTS:
	void reconnect();
	void disconnect();
	void resync();

	void listDevices();
	void removeDevice(const QUuid &deviceId);
	void resetAccount();
	void importAccount(const ExportData &data, bool keepData);
	void loginReply(const QUuid &deviceId, bool accept);

	void uploadData(const QByteArray &key, const QByteArray &changeData);
	void downloadDone(const quint64 key);

	void setSyncEnabled(bool syncEnabled);
	void setDeviceName(const QString &deviceName);
	void resetDeviceName();

Q_SIGNALS:
	void finalized();
	void remoteEvent(RemoteEvent event);
	void uploadDone(const QByteArray &key);
	void downloadData(const quint64 key, const QByteArray &changeData);

	void syncEnabledChanged(bool syncEnabled);
	void deviceNameChanged(const QString &deviceName);
	void devicesListed(const QList<DeviceInfo> &devices);
	void loginRequested(const DeviceInfo &deviceInfo);

private Q_SLOTS:
	void connected();
	void disconnected();
	void binaryMessageReceived(const QByteArray &message);
	void error(QAbstractSocket::SocketError error);
	void sslErrors(const QList<QSslError> &errors);
	void ping();

	//statemachine
	void doConnect();
	void doDisconnect();
	void scheduleRetry();
	void onEntryIdleState();
	void onExitActiveState();

private:
	static const QVector<std::chrono::seconds> Timeouts;

	CryptoController *_cryptoController;

	QWebSocket *_socket;

	QTimer *_pingTimer;
	bool _awaitingPing;

	ConnectorStateMachine *_stateMachine;
	int _retryIndex;
	bool _expectChanges;
	//TODO timeout for server downloads?

	QUuid _deviceId;
	mutable QList<DeviceInfo> _deviceCache;
	QSet<QByteArray> _exportsCache;

	bool isIdle() const;
	void triggerError(bool canRecover);

	bool checkCanSync(QUrl &remoteUrl);
	bool loadIdentity();
	void tryClose();
	std::chrono::seconds retry();

	QVariant sValue(const QString &key) const;

	void onError(const ErrorMessage &message);
	void onIdentify(const IdentifyMessage &message);
	void onAccount(const AccountMessage &message);
	void onWelcome(const WelcomeMessage &message);
	void onChangeAck(const ChangeAckMessage &message);
	void onChanged(const ChangedMessage &message);
	void onChangedInfo(const ChangedInfoMessage &message);
	void onLastChanged(const LastChangedMessage &message);
	void onDevices(const DevicesMessage &message);
	void onRemoved(const RemovedMessage &message);
};

}

#endif // REMOTECONNECTOR_P_H
