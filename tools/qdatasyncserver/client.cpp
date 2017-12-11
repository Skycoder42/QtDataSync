#include "client.h"
#include "app.h"
#include "identifymessage_p.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QtConcurrent>

using namespace QtDataSync;

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

	//initialize connection by sending indent message
	socket->sendBinaryMessage(serializeMessage(IdentifyMessage::createRandom()));
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
	QtConcurrent::run(qApp->threadPool(), [message, this]() {
		try {
			QDataStream stream(message);
			setupStream(stream);
			stream.startTransaction();
			QByteArray name;
			stream >> name;
			if(!stream.commitTransaction())
				throw DataStreamException(stream);

			if(isType<RegisterMessage>(name)) {
				auto msg = deserializeMessage<RegisterMessage>(stream);
				if(msg.keyAlgorithm == RsaScheme::StaticAlgorithmName())
					verifySignature(stream, msg.getKey<RsaScheme>());
				else
					throw CryptoPP::Exception(CryptoPP::Exception::INVALID_DATA_FORMAT, "Key algorithm " + msg.keyAlgorithm + "is not supported");
				onRegister(msg);
			} else {
				qWarning() << "Unknown message received: " << message;
				close();
			}
		} catch(DataStreamException &e) {
			qWarning() << "Client message error:" << e.what();
			close();
		} catch(CryptoPP::Exception &e) {
			qWarning() << "Client message error:" << e.what();
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
	if(socket->state() == QAbstractSocket::ConnectedState)
		socket->close();
}

void Client::sslErrors(const QList<QSslError> &errors)
{
	foreach(auto error, errors) {
		qWarning() << socket->peerAddress()
				   << "SSL errors"
				   << error.errorString();
	}
	if(socket->state() == QAbstractSocket::ConnectedState)
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

void Client::sendMessage(const QByteArray &message)
{
	QMetaObject::invokeMethod(this, "doSend", Qt::QueuedConnection,
							  Q_ARG(QByteArray, message));
}

void Client::doSend(const QByteArray &message)
{
	socket->sendBinaryMessage(message);
}

void Client::onRegister(const RegisterMessage &message)
{
	qDebug() << Q_FUNC_INFO << message;
}
