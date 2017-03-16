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
	data[QStringLiteral("type")] = type;
	data[QStringLiteral("key")] = key;
	data[QStringLiteral("changed")] = changed;
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
		auto data = obj[QStringLiteral("data")];
		if(obj[QStringLiteral("command")] == QStringLiteral("createIdentity"))
			createIdentity(data.toObject());
		else if(obj[QStringLiteral("command")] == QStringLiteral("identify"))
			identify(data.toObject());
		else if(obj[QStringLiteral("command")] == QStringLiteral("loadChanges"))
			loadChanges(data.toBool());
		else if(obj[QStringLiteral("command")] == QStringLiteral("load"))
			load(data.toObject());
		else if(obj[QStringLiteral("command")] == QStringLiteral("save"))
			save(data.toObject());
		else if(obj[QStringLiteral("command")] == QStringLiteral("remove"))
			remove(data.toObject());
		else if(obj[QStringLiteral("command")] == QStringLiteral("markUnchanged"))
			markUnchanged(data.toObject());
		else if(obj[QStringLiteral("command")] == QStringLiteral("deleteOldDevice"))
			deleteOldDevice();
		else {
			qDebug() << "Unknown command"
					 << obj[QStringLiteral("command")].toString();
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
	devId = QUuid(data[QStringLiteral("deviceId")].toString());
	if(devId.isNull()) {
		close();
		return;
	}

	auto resync = false;
	auto identity = database->createIdentity(devId, resync);
	if(!identity.isNull()) {
		userId = identity;
		qInfo() << "Created new identity"
				<< userId.toByteArray().constData()
				<< "for"
				<< socketAddress;
		QJsonObject reply;
		reply[QStringLiteral("userId")] = userId.toString();
		reply[QStringLiteral("resync")] = resync;
		sendCommand("identified", reply);
		emit connected(userId, true);
	}  else {
		sendCommand("identifyFailed");
		close();
	}
}

void Client::identify(const QJsonObject &data)
{
	devId = QUuid(data[QStringLiteral("deviceId")].toString());
	if(devId.isNull()) {
		close();
		return;
	}
	userId = QUuid(data[QStringLiteral("userId")].toString());
	if(userId.isNull()) {
		close();
		return;
	}

	auto resync = false;
	if(database->identify(userId, devId, resync)) {
		QJsonObject reply;
		reply[QStringLiteral("userId")] = userId.toString();
		reply[QStringLiteral("resync")] = resync;
		sendCommand("identified", reply);
		emit connected(userId, false);
	} else {
		qWarning() << "Invalid user data from"
				   << socketAddress;
		sendCommand("identifyFailed");
		close();
	}
}

void Client::loadChanges(bool resync)
{
	auto changes = database->loadChanges(userId, devId, resync);
	QJsonObject reply;
	if(!changes.isNull()) {
		reply[QStringLiteral("success")] = true;
		reply[QStringLiteral("data")] = changes;
		reply[QStringLiteral("error")] = QJsonValue::Null;
	} else  {
		reply[QStringLiteral("success")] = false;
		reply[QStringLiteral("data")] = QJsonValue::Null;
		reply[QStringLiteral("error")] = QStringLiteral("Failed to load state from server database");
	}
	sendCommand("changeState", reply);
}

void Client::load(const QJsonObject &data)
{
	auto type = data[QStringLiteral("type")].toString();
	auto key = data[QStringLiteral("key")].toString();

	QJsonObject reply;
	if(type.isEmpty() || key.isEmpty()) {
		reply[QStringLiteral("success")] = false;
		reply[QStringLiteral("data")] = QJsonValue::Null;
		reply[QStringLiteral("error")] = QStringLiteral("Invalid type or key!");
	} else {
		auto replyData = database->load(userId, type, key);
		if(!replyData.isNull()) {
			reply[QStringLiteral("success")] = true;
			reply[QStringLiteral("data")] = replyData;
			reply[QStringLiteral("error")] = QJsonValue::Null;
		} else  {
			reply[QStringLiteral("success")] = false;
			reply[QStringLiteral("data")] = QJsonValue::Null;
			reply[QStringLiteral("error")] = QStringLiteral("Failed to load data from server database");
		}
	}
	sendCommand("completed", reply);
}

void Client::save(const QJsonObject &data)
{
	auto type = data[QStringLiteral("type")].toString();
	auto key = data[QStringLiteral("key")].toString();
	auto value = data[QStringLiteral("value")].toObject();

	QJsonObject reply;
	if(type.isEmpty() || key.isEmpty()) {
		reply[QStringLiteral("success")] = false;
		reply[QStringLiteral("error")] = QStringLiteral("Invalid type or key!");
	} else if(database->save(userId, devId, type, key, value)) {
		reply[QStringLiteral("success")] = true;
		reply[QStringLiteral("error")] = QJsonValue::Null;
	} else  {
		reply[QStringLiteral("success")] = false;
		reply[QStringLiteral("error")] = QStringLiteral("Failed to save data on server database");
	}
	sendCommand("completed", reply);
}

void Client::remove(const QJsonObject &data)
{
	auto type = data[QStringLiteral("type")].toString();
	auto key = data[QStringLiteral("key")].toString();

	QJsonObject reply;
	if(type.isEmpty() || key.isEmpty()) {
		reply[QStringLiteral("success")] = false;
		reply[QStringLiteral("error")] = QStringLiteral("Invalid type or key!");
	} else if(database->remove(userId, devId, type, key)) {
		reply[QStringLiteral("success")] = true;
		reply[QStringLiteral("error")] = QJsonValue::Null;
	} else  {
		reply[QStringLiteral("success")] = false;
		reply[QStringLiteral("error")] = QStringLiteral("Failed to remove data from server database");
	}
	sendCommand("completed", reply);
}

void Client::markUnchanged(const QJsonObject &data)
{
	auto type = data[QStringLiteral("type")].toString();
	auto key = data[QStringLiteral("key")].toString();

	QJsonObject reply;
	if(type.isEmpty() || key.isEmpty()) {
		reply[QStringLiteral("success")] = false;
		reply[QStringLiteral("error")] = QStringLiteral("Invalid type or key!");
	} else if(database->markUnchanged(userId, devId, type, key)) {
		reply[QStringLiteral("success")] = true;
		reply[QStringLiteral("error")] = QJsonValue::Null;
	} else  {
		reply[QStringLiteral("success")] = false;
		reply[QStringLiteral("error")] = QStringLiteral("Failed to mark as unchanged on server database");
	}
	sendCommand("completed", reply);
}

void Client::deleteOldDevice()
{
	database->deleteOldDevice(userId, devId);
}

void Client::close()
{
	QMetaObject::invokeMethod(socket, "close");
}

void Client::sendCommand(const QByteArray &command, const QJsonValue &data)
{
	QJsonObject message;
	message[QStringLiteral("command")] = QString::fromUtf8(command);
	message[QStringLiteral("data")] = data;

	QJsonDocument doc(message);
	QMetaObject::invokeMethod(this, "doSend", Q_ARG(QByteArray, doc.toJson(QJsonDocument::Compact)));
}

void Client::doSend(const QByteArray &message)
{
	socket->sendBinaryMessage(message);
}
