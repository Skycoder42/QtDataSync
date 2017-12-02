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
	Q_UNIMPLEMENTED();
}

void Client::binaryMessageReceived(const QByteArray &message)
{
	runCount++;
	QtConcurrent::run(qApp->threadPool(), [message, this](){
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
		/*else*/ {
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
	if(runCount == 0)//save close -> delete only if no parallel stuff anymore (check every 500 ms)
		deleteLater();
	else {
		auto destroyTimer = new QTimer(this);
		connect(destroyTimer, &QTimer::timeout, this, [this](){
			if(runCount == 0)
				deleteLater();
		});
		destroyTimer->start(500);
	}
}

void Client::close()
{
	QMetaObject::invokeMethod(socket, "close", Qt::QueuedConnection);
}

void Client::sendCommand(const QByteArray &command, const QJsonValue &data)
{
	QJsonObject message;
	message[QStringLiteral("command")] = QString::fromUtf8(command);
	message[QStringLiteral("data")] = data;

	QJsonDocument doc(message);
	QMetaObject::invokeMethod(this, "doSend", Qt::QueuedConnection,
							  Q_ARG(QByteArray, doc.toJson(QJsonDocument::Compact)));
}

void Client::doSend(const QByteArray &message)
{
	socket->sendBinaryMessage(message);
}
