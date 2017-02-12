#include "client.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

Client::Client(DatabaseController *database, QWebSocket *websocket, QObject *parent) :
	QObject(parent),
	database(database),
	socket(websocket),
	clientId(),
	deviceId()
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
	auto data = obj["data"];
	if(obj["command"] == QStringLiteral("createIdentity"))
		createIdentity(data.toObject());
	else if(obj["command"] == QStringLiteral("identify"))
		identify(data.toObject());
	else {
		qDebug() << "Unknown command"
				 << obj["command"].toString();
		socket->close();
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

void Client::createIdentity(const QJsonObject &data)
{
	deviceId = QUuid(data["deviceId"].toString());
	if(deviceId.isNull()) {
		socket->close();
		return;
	}

	database->createIdentity(this, "createIdentityResult");
}

void Client::createIdentityResult(QUuid identity)
{
	if(!identity.isNull()) {
		clientId = identity;
		qInfo() << "Created new Identity"
				<< clientId.toByteArray().constData()
				<< "for"
				<< socket->peerAddress();
		sendCommand("identity", clientId.toString());

		emit connected(deviceId, true);
	} else {
		socket->close();
		return;
	}
}

void Client::identify(const QJsonObject &data)
{
	deviceId = QUuid(data["deviceId"].toString());
	if(deviceId.isNull()) {
		socket->close();
		return;
	}
	clientId = QUuid(data["userId"].toString());
	if(clientId.isNull()) {
		socket->close();
		return;
	}

	database->identify(clientId, this, "identifyResult");
}

void Client::identifyResult(bool ok)
{
	if(ok)
		emit connected(deviceId, false);
	else
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
