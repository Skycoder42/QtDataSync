#ifndef REMOTECONNECTOR_P_H
#define REMOTECONNECTOR_P_H

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QTimer>

#include <QtWebSockets/QWebSocket>

#include "qtdatasync_global.h"
#include "controller_p.h"
#include "defaults.h"
#include "cryptocontroller_p.h"

#include "identifymessage_p.h"
#include "accountmessage_p.h"
#include "welcomemessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT RemoteConnector : public Controller
{
	Q_OBJECT

public:
	static const QString keyRemoteEnabled;
	static const QString keyRemoteUrl;
	static const QString keyAccessKey;
	static const QString keyHeaders;
	static const QString keyKeepaliveTimeout;
	static const QString keyDeviceId;
	static const QString keyDeviceName;

	//! Describes the current state of the connector
	enum RemoteState {
		RemoteDisconnected,
		RemoteReconnecting,
		RemoteConnected,
		RemoteRegistering,
		RemoteLoggingIn,
		RemoteLoading,
		RemoteReady
	};
	Q_ENUM(RemoteState)

	explicit RemoteConnector(const Defaults &defaults, QObject *parent = nullptr);

	void initialize() final;
	void finalize() final;

public Q_SLOTS:
	void reconnect();
	void reloadState();

Q_SIGNALS:
	void stateChanged(RemoteState state);

private Q_SLOTS:
	void connected();
	void disconnected();
	void binaryMessageReceived(const QByteArray &message);
	void error(QAbstractSocket::SocketError error);
	void sslErrors(const QList<QSslError> &errors);
	void timeout();

private:
	static const QByteArray PingMessage;

	CryptoController *_cryptoController;

	QWebSocket *_socket;
	QTimer *_idleTimer;
	bool _changingConnection;
	RemoteState _state;

	QUuid _deviceId;

	bool checkCanSync(QUrl &remoteUrl);
	bool loadIdentity();
	void tryClose();

	QVariant sValue(const QString &key) const;
	void upState(RemoteState state);

	void onIdentify(const IdentifyMessage &message);
	void onAccount(const AccountMessage &message);
	void onWelcome(const WelcomeMessage &message);
};

}

#endif // REMOTECONNECTOR_P_H
