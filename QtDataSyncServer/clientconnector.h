#ifndef CLIENTCONNECTOR_H
#define CLIENTCONNECTOR_H

#include "client.h"

#include <QObject>
#include <QWebSocketServer>

class ClientConnector : public QObject
{
	Q_OBJECT
public:
	explicit ClientConnector(const QString &name, bool wss, QObject *parent = nullptr);

	bool setupWss(const QString &p12File, const QString &passphrase);
	bool listen(const QHostAddress &hostAddress, quint16 port);

private slots:
	void newConnection();
	void serverError();
	void sslErrors(const QList<QSslError> &errors);

private:
	QWebSocketServer *server;

	QHash<QUuid, Client*> clients;
};

#endif // CLIENTCONNECTOR_H
