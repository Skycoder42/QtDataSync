#ifndef CLIENTCONNECTOR_H
#define CLIENTCONNECTOR_H

#include "client.h"
#include "databasecontroller.h"

#include <QObject>
#include <QWebSocketServer>

class ClientConnector : public QObject
{
	Q_OBJECT
public:
	explicit ClientConnector(DatabaseController *database, QObject *parent = nullptr);

	void recreateServer();
	bool setupWss();
	bool listen();
	void close();

	void setPaused(bool paused);

public Q_SLOTS:
	void notifyChanged(QUuid deviceId);

Q_SIGNALS:
	void disconnectAll();

private Q_SLOTS:
	void verifySecret(QWebSocketCorsAuthenticator *authenticator);
	void newConnection();
	void serverError();
	void sslErrors(const QList<QSslError> &errors);

	void clientConnected(QUuid deviceId);
	void proofRequested(QUuid partner, const QtDataSync::ProofMessage &message);
	void forceDisconnect(QUuid partner);

private:
	DatabaseController *database;
	QWebSocketServer *server = nullptr;
	QString secret;
	bool isActivated = false;

	QHash<QUuid, Client*> clients;
};

#endif // CLIENTCONNECTOR_H
