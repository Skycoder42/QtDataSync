#ifndef CLIENTCONNECTOR_H
#define CLIENTCONNECTOR_H

#include "client.h"

#include <QObject>
#include <QWebSocketServer>

class ClientConnector : public QObject
{
	Q_OBJECT
public:
	explicit ClientConnector(QObject *parent = nullptr);

	bool setupWss();
	bool listen();

private slots:
	void newConnection();
	void serverError();
	void sslErrors(const QList<QSslError> &errors);

private:
	QWebSocketServer *server;

	QHash<QUuid, Client*> clients;
};

#endif // CLIENTCONNECTOR_H
