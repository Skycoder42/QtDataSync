#include "clientconnector.h"
#include <QWebSocket>

ClientConnector::ClientConnector(const QString &name, QObject *parent) :
	QObject(parent),
	server(new QWebSocketServer(name, QWebSocketServer::NonSecureMode, this)),
	clients()
{
	connect(server, &QWebSocketServer::newConnection,
			this, &ClientConnector::newConnection);
	connect(server, &QWebSocketServer::serverError,
			this, &ClientConnector::serverError);
	connect(server, &QWebSocketServer::sslErrors,
			this, &ClientConnector::sslErrors);
}

bool ClientConnector::listen(const QHostAddress &hostAddress, quint16 port)
{
	if(server->listen(hostAddress, port)) {
		qInfo() << "Listening on port" << server->serverPort();
		return true;
	} else {
		qCritical() << "Failed to create server with error:"
					<< server->errorString();
		return false;
	}
}

void ClientConnector::newConnection()
{
	while (server->hasPendingConnections()) {
		auto socket = server->nextPendingConnection();
		new Client(socket, this);
	}
}

void ClientConnector::serverError()
{
	qWarning() << "Server error"
			   << server->errorString();
}

void ClientConnector::sslErrors(const QList<QSslError> &errors)
{
	foreach(auto error, errors) {
		qWarning() << "SSL errors"
				   << error.errorString();
	}
}
