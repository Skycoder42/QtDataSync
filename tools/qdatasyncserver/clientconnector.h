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

public slots:
	void notifyChanged(const QUuid &deviceId);

private slots:
	void verifySecret(QWebSocketCorsAuthenticator *authenticator);
	void newConnection();
	void serverError();
	void sslErrors(const QList<QSslError> &errors);

private:
	DatabaseController *database;
	QWebSocketServer *server;
	QString secret;

	QMultiHash<QUuid, Client*> clients;
};

#endif // CLIENTCONNECTOR_H
