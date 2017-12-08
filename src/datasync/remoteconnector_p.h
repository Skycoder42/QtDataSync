#ifndef REMOTECONNECTOR_P_H
#define REMOTECONNECTOR_P_H

#include <QtCore/QObject>

#include <QtWebSockets/QWebSocket>

#include "qtdatasync_global.h"
#include "controller_p.h"
#include "defaults.h"
#include "cryptocontroller_p.h"

#include "identifymessage_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT RemoteConnector : public Controller
{
	Q_OBJECT

public:
	//! Describes the current state of the connector
	enum RemoteState {
		RemoteDisconnected,//!< The remote server is "not available"
		RemoteReconnecting,//!< The controller is currently trying to connect to the remote (may result in disconnected or loading states)
		RemoteLoadingState,//!< The remote is available (or trying to connect to it), and the remote state is currently beeing loaded
		RemoteReady//!< The remote state was successfully loaded and the connector is ready for synchronization
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

private:
	static const QString keyRemoteUrl;
	static const QString keyAccessKey;
	static const QString keyHeaders;
	static const QString keyUserId;

	CryptoController *_cryptoController;

	QWebSocket *_socket;
	bool _changingConnection;

	void tryClose();

	QVariant sValue(const QString &key) const;

	void onIdentify(const IdentifyMessage &message);
};

}

#endif // REMOTECONNECTOR_P_H
