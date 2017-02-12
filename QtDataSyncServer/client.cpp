#include "client.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

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

QUuid Client::userId() const
{
	return clientId;
}

void Client::binaryMessageReceived(const QByteArray &message)
{
	QJsonParseError error;
	auto doc = QJsonDocument::fromJson(message, &error);
	if(error.error != QJsonParseError::NoError) {
		qWarning() << "Invalid data received. Parser error:"
				   << error.errorString();
		return;
	}

	auto obj = doc.object();
	auto data = obj["data"].toObject();
	if(obj["command"] == QStringLiteral("createIdentity")) {
		QUuid devId(data["deviceId"].toString());
		if(devId.isNull())
			return;

		clientId = QUuid::createUuid();
		qInfo() << "Created new Identity"
				<< clientId.toByteArray().constData()
				<< "for"
				<< socket->peerAddress();
		sendCommand("identity", clientId.toString());

		emit connected(devId, true);
	} else if(obj["command"] == QStringLiteral("identify")) {
		QUuid devId(data["deviceId"].toString());
		if(devId.isNull())
			return;
		clientId = QUuid(data["userId"].toString());
		if(clientId.isNull())
			return;

		emit connected(devId, false);
	} else {
		qDebug() << "Unknown command"
				 << obj["command"].toString();
	}
}

void Client::error()
{
	qWarning() << socket->peerAddress()
			   << "Socket error"
			   << socket->errorString();
	socket->close();
}

void Client::sslErrors(const QList<QSslError> &errors)
{
	foreach(auto error, errors) {
		qWarning() << socket->peerAddress()
				   << "SSL errors"
				   << error.errorString();
	}
	socket->close();
}

void Client::sendCommand(const QByteArray &command, const QJsonValue &data)
{
	QJsonObject message;
	message["command"] = QString::fromLatin1(command);
	message["data"] = data;

	QJsonDocument doc(message);
	socket->sendBinaryMessage(doc.toJson(QJsonDocument::Compact));
}
