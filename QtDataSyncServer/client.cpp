#include "client.h"
#include "app.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QtConcurrent>

Client::Client(DatabaseController *database, QWebSocket *websocket, QObject *parent) :
	QObject(parent),
	database(database),
	socket(websocket),
	userId(),
	devId(),
	socketAddress(socket->peerAddress()),
	runCount(0)
{
	socket->setParent(this);

	connect(socket, &QWebSocket::disconnected,
			this, &Client::closeClient);
	connect(socket, &QWebSocket::binaryMessageReceived,
			this, &Client::binaryMessageReceived);
	connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
			this, &Client::error);
	connect(socket, &QWebSocket::sslErrors,
			this, &Client::sslErrors);
}

QUuid Client::deviceId() const
{
	return devId;
}

void Client::notifyChanged(const QString &type, const QString &key, bool changed)
{
	QJsonObject data;
	data["type"] = type;
	data["key"] = key;
	data["changed"] = changed;
	sendCommand("notifyChanged", data);
}

void Client::binaryMessageReceived(const QByteArray &message)
{
	runCount++;
	QtConcurrent::run(qApp->threadPool(), [=](){
		QJsonParseError error;
		auto doc = QJsonDocument::fromJson(message, &error);
		if(error.error != QJsonParseError::NoError) {
			qWarning() << "Invalid data received. Parser error:"
					   << error.errorString();
			runCount--;
			return;
		}

		auto obj = doc.object();
		auto data = obj["data"];
		if(obj["command"] == QStringLiteral("createIdentity"))
			createIdentity(data.toObject());
		else if(obj["command"] == QStringLiteral("identify"))
			identify(data.toObject());
		else if(obj["command"] == QStringLiteral("loadChanges"))
			loadChanges();
		else if(obj["command"] == QStringLiteral("load"))
			load(data.toObject());
		else if(obj["command"] == QStringLiteral("save"))
			save(data.toObject());
		else if(obj["command"] == QStringLiteral("remove"))
			remove(data.toObject());
		else if(obj["command"] == QStringLiteral("markUnchanged"))
			markUnchanged(data.toObject());
		else {
			qDebug() << "Unknown command"
					 << obj["command"].toString();
			close();
		}

		runCount--;
	});
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

void Client::closeClient()
{
	if(runCount == 0)//save close -> delete only if no parallel stuff anymore
		this->deleteLater();
	else {
		auto destroyTimer = new QTimer(this);
		connect(destroyTimer, &QTimer::timeout, this, [=](){
			if(runCount == 0)
				this->deleteLater();
		});
		destroyTimer->start(500);
	}
}

void Client::createIdentity(const QJsonObject &data)
{
	devId = QUuid(data["deviceId"].toString());
	if(devId.isNull()) {
		close();
		return;
	}

	auto identity = database->createIdentity(devId);
	if(!identity.isNull()) {
		userId = identity;
		qInfo() << "Created new identity"
				<< userId.toByteArray().constData()
				<< "for"
				<< socketAddress;
		sendCommand("identified", userId.toString());
		emit connected(userId, true);
	} else {
		close();
		return;
	}
}

void Client::identify(const QJsonObject &data)
{
	devId = QUuid(data["deviceId"].toString());
	if(devId.isNull()) {
		close();
		return;
	}
	userId = QUuid(data["userId"].toString());
	if(userId.isNull()) {
		close();
		return;
	}

	if(database->identify(userId, devId)) {
		sendCommand("identified", userId.toString());
		emit connected(userId, false);
	} else
		close();
}

void Client::loadChanges()
{
	auto changes = database->loadChanges(userId, devId);
	QJsonObject reply;
	if(!changes.isNull()) {
		reply["success"] = true;
		reply["data"] = changes;
		reply["error"] = QJsonValue::Null;
	} else  {
		reply["success"] = false;
		reply["data"] = QJsonValue::Null;
		reply["error"] = "Failed to load state from server database";
	}
	sendCommand("changeState", reply);
}

void Client::load(const QJsonObject &data)
{
	auto type = data["type"].toString();
	auto key = data["key"].toString();

	QJsonObject reply;
	if(type.isEmpty() || key.isEmpty()) {
		reply["success"] = false;
		reply["data"] = QJsonValue::Null;
		reply["error"] = "Invalid type or key!";
	} else {
		auto replyData = database->load(userId, type, key);
		if(!replyData.isNull()) {
			reply["success"] = true;
			reply["data"] = replyData;
			reply["error"] = QJsonValue::Null;
		} else  {
			reply["success"] = false;
			reply["data"] = QJsonValue::Null;
			reply["error"] = "Failed to load data from server database";
		}
	}
	sendCommand("completed", reply);
}

void Client::save(const QJsonObject &data)
{
	auto type = data["type"].toString();
	auto key = data["key"].toString();
	auto value = data["value"].toObject();

	QJsonObject reply;
	if(type.isEmpty() || key.isEmpty()) {
		reply["success"] = false;
		reply["error"] = "Invalid type or key!";
	} else if(database->save(userId, devId, type, key, value)) {
		reply["success"] = true;
		reply["error"] = QJsonValue::Null;
	} else  {
		reply["success"] = false;
		reply["error"] = "Failed to save data on server database";
	}
	sendCommand("completed", reply);
}

void Client::remove(const QJsonObject &data)
{
	auto type = data["type"].toString();
	auto key = data["key"].toString();

	QJsonObject reply;
	if(type.isEmpty() || key.isEmpty()) {
		reply["success"] = false;
		reply["error"] = "Invalid type or key!";
	} else if(database->remove(userId, devId, type, key)) {
		reply["success"] = true;
		reply["error"] = QJsonValue::Null;
	} else  {
		reply["success"] = false;
		reply["error"] = "Failed to remove data from server database";
	}
	sendCommand("completed", reply);
}

void Client::markUnchanged(const QJsonObject &data)
{
	auto type = data["type"].toString();
	auto key = data["key"].toString();

	QJsonObject reply;
	if(type.isEmpty() || key.isEmpty()) {
		reply["success"] = false;
		reply["error"] = "Invalid type or key!";
	} else if(database->markUnchanged(userId, devId, type, key)) {
		reply["success"] = true;
		reply["error"] = QJsonValue::Null;
	} else  {
		reply["success"] = false;
		reply["error"] = "Failed to mark as unchanged on server database";
	}
	sendCommand("completed", reply);
}

void Client::close()
{
	QMetaObject::invokeMethod(socket, "close");
}

void Client::sendCommand(const QByteArray &command, const QJsonValue &data)
{
	QJsonObject message;
	message["command"] = QString::fromLatin1(command);
	message["data"] = data;

	QJsonDocument doc(message);
	QMetaObject::invokeMethod(this, "doSend", Q_ARG(QByteArray, doc.toJson(QJsonDocument::Compact)));
}

void Client::doSend(const QByteArray &message)
{
	socket->sendBinaryMessage(message);
}
