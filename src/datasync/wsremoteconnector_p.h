#ifndef QTDATASYNC_WSREMOTECONNECTOR_P_H
#define QTDATASYNC_WSREMOTECONNECTOR_P_H

#include "qtdatasync_global.h"
#include "remoteconnector.h"

#include <QtCore/QJsonArray>
#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <QtWebSockets/QWebSocket>

namespace QtDataSync {

class Q_DATASYNC_EXPORT WsRemoteConnector : public RemoteConnector
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

	static const QString keyRemoteEnabled;
	static const QString keyRemoteUrl;
	static const QString keyHeadersGroup;
	static const QString keyVerifyPeer;
	static const QString keyUserIdentity;
	static const QString keySharedSecret;

	explicit WsRemoteConnector(QObject *parent = nullptr);

	void initialize(Defaults *defaults, Encryptor *cryptor) override;
	void finalize() override;

	Authenticator *createAuthenticator(Defaults *defaults, QObject *parent) override;

public Q_SLOTS:
	void reconnect();
	void reloadRemoteState() override;
	void requestResync() override;

	void download(const ObjectKey &key, const QByteArray &keyProperty) override;
	void upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) override;
	void remove(const ObjectKey &key, const QByteArray &keyProperty) override;
	void markUnchanged(const ObjectKey &key, const QByteArray &keyProperty) override;

protected:
	void resetUserData(const QVariant &extraData, const QByteArray &) override;
	void timerEvent(QTimerEvent *event) override;

private Q_SLOTS:
	void connected();
	void disconnected();
	void binaryMessageReceived(const QByteArray &message);
	void error();
	void sslErrors(const QList<QSslError> &errors);

	void sendCommand(const QByteArray &command, const QJsonValue &data = QJsonValue::Null);

	void operationTimeout();

private:
	static const QVector<int> timeouts;

	QWebSocket *socket;
	QSettings *settings;

	SocketState state;
	int retryIndex;
	bool needResync;

	QTimer *operationTimer;
	int pingTimerId;

	bool decryptReply;
	ObjectKey currentKey;
	QByteArray currentKeyProperty;

	QJsonObject keyObject(const ObjectKey &key) const;

	void identified(const QJsonObject &data);
	void identifyFailed();
	void changeState(const QJsonObject &data);
	void notifyChanged(const QJsonObject &data);
	void completed(const QJsonObject &result);

	int retry();
};

}

#endif // QTDATASYNC_WSREMOTECONNECTOR_P_H
