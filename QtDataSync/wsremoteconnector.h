#ifndef WSREMOTECONNECTOR_H
#define WSREMOTECONNECTOR_H

#include "remoteconnector.h"

#include <QObject>
#include <QSettings>
#include <QWebSocket>

namespace QtDataSync {

class WsRemoteConnector : public RemoteConnector
{
	Q_OBJECT
public:
	enum SocketState {
		Disconnected,
		Connecting,
		Identifying,
		Reloading,
		Idle,
		Closing
	};
	Q_ENUM(SocketState)

	static const QString keyRemoteUrl;
	static const QString keyHeadersGroup;
	static const QString keyVerifyPeer;
	static const QString keyUserIdentity;

	explicit WsRemoteConnector(QObject *parent = nullptr);

	void initialize(const QDir &storageDir) override;
	void finalize(const QDir &storageDir) override;

	Authenticator *createAuthenticator(const QDir &storageDir, QObject *parent) override;

public slots:
	void reconnect();

	void download(const ObjectKey &key, const QByteArray &keyProperty) override;
	void upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) override;
	void remove(const ObjectKey &key, const QByteArray &keyProperty) override;
	void markUnchanged(const ObjectKey &key, const QByteArray &keyProperty) override;

	void resetDeviceId() override;

private slots:
	void connected();
	void disconnected();
	void binaryMessageReceived(const QByteArray &message);
	void error();
	void sslErrors(const QList<QSslError> &errors);

	void sendCommand(const QByteArray &command, const QJsonValue &data = QJsonValue::Null);

private:
	QWebSocket *socket;
	QDir storageDir;
	QSettings *settings;

	SocketState state;
};

}

#endif // WSREMOTECONNECTOR_H
