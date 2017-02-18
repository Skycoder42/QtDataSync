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
		Idle,
		Reloading,
		Operating,
		Closing
	};
	Q_ENUM(SocketState)

	static const QString keyRemoteUrl;
	static const QString keyHeadersGroup;
	static const QString keyVerifyPeer;
	static const QString keyUserIdentity;

	explicit WsRemoteConnector(QObject *parent = nullptr);

	void initialize(Defaults *defaults) override;
	void finalize() override;

	Authenticator *createAuthenticator(Defaults *defaults, QObject *parent) override;

public slots:
	void reconnect();
	void reload();

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
	QSettings *settings;

	SocketState state;

	QJsonObject keyObject(const ObjectKey &key) const;

	void identified(const QString &data);
	void saved(const QJsonObject &result);
};

}

#endif // WSREMOTECONNECTOR_H
