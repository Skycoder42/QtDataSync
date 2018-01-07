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

	bool setupWss();
	bool listen();

public Q_SLOTS:
	void notifyChanged(const QUuid &deviceId);

private Q_SLOTS:
	void verifySecret(QWebSocketCorsAuthenticator *authenticator);
	void newConnection();
	void serverError();
	void sslErrors(const QList<QSslError> &errors);

	void clientConnected(const QUuid &deviceId);
	void proofRequested(const QUuid &partner, const QtDataSync::ProofMessage &message);
	void keyDisconnect(const QUuid &partner);

private:
	DatabaseController *database;
	QWebSocketServer *server;
	QString secret;

	QHash<QUuid, Client*> clients;
};

#endif // CLIENTCONNECTOR_H
