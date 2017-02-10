#include "client.h"

Client::Client(QWebSocket *websocket, QObject *parent) :
	QObject(parent),
	socket(websocket)
{
	socket->setParent(this);

	connect(socket, &QWebSocket::disconnected,
			this, &Client::deleteLater);
	connect(socket, &QWebSocket::binaryMessageReceived,
			this, &Client::binaryMessageReceived);
	connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
			this, &Client::error);
	connect(socket, &QWebSocket::sslErrors,
			this, &Client::sslErrors);
}

void Client::binaryMessageReceived(const QByteArray &message)
{
	qDebug() << message;
}

void Client::error()
{
	qWarning() << "Server error"
			   << socket->errorString();
	socket->close();
}

void Client::sslErrors(const QList<QSslError> &errors)
{
	foreach(auto error, errors) {
		qWarning() << "SSL errors"
				   << error.errorString();
	}
	socket->close();
}
