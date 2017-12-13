#include "client.h"
#include "app.h"
#include "identifymessage_p.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QtConcurrent>

using namespace QtDataSync;

QThreadStorage<CryptoPP::AutoSeededRandomPool> Client::rngPool;

#define LOCK QMutexLocker _(&_propMutex)

Client::Client(DatabaseController *database, QWebSocket *websocket, QObject *parent) :
	QObject(parent),
	_database(database),
	_socket(websocket),
	_deviceId(),
	_runCount(0),
	_propMutex(),
	_propHash()
{
	_socket->setParent(this);

	connect(_socket, &QWebSocket::disconnected,
			this, &Client::closeClient);
	connect(_socket, &QWebSocket::binaryMessageReceived,
			this, &Client::binaryMessageReceived);
	connect(_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
			this, &Client::error);
	connect(_socket, &QWebSocket::sslErrors,
			this, &Client::sslErrors);

	_runCount++;
	QtConcurrent::run(qApp->threadPool(), [this]() {
		LOCK;
		//initialize connection by sending indent message
		auto msg = IdentifyMessage::createRandom(rngPool.localData());
		_propHash.insert("nonce", msg.nonce);
		sendMessage(serializeMessage(msg));
		_runCount--;
	});
}

QUuid Client::deviceId() const
{
	return _deviceId;
}

void Client::notifyChanged(const QString &type, const QString &key, bool changed)
{
	Q_UNIMPLEMENTED();
}

void Client::binaryMessageReceived(const QByteArray &message)
{
	_runCount++;
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
				QScopedPointer<AsymmetricCrypto> crypto(msg.createCrypto(nullptr));
				verifySignature(stream, msg.getSignKey(rngPool.localData(), crypto.data()), crypto.data());
				onRegister(msg);
			} else {
				qWarning() << "Unknown message received: " << message;
				close();
			}
		} catch(DatabaseException &e) {
			qWarning().noquote() << "Internal database error:" << e.errorString()
								 << "\nResettings connection in hopes to fix it.";
			close();
		} catch(std::exception &e) {
			qWarning() << "Client message error:" << e.what();
			close();
		}

		_runCount--;
	});
}

void Client::error()
{
	qWarning() << _socket->peerAddress()
			   << "Socket error"
			   << _socket->errorString();
	if(_socket->state() == QAbstractSocket::ConnectedState)
		_socket->close();
}

void Client::sslErrors(const QList<QSslError> &errors)
{
	foreach(auto error, errors) {
		qWarning() << _socket->peerAddress()
				   << "SSL errors"
				   << error.errorString();
	}
	if(_socket->state() == QAbstractSocket::ConnectedState)
		_socket->close();
}

void Client::closeClient()
{
	if(_runCount == 0)//save close -> delete only if no parallel stuff anymore (check every 500 ms)
		deleteLater();
	else {
		auto destroyTimer = new QTimer(this);
		connect(destroyTimer, &QTimer::timeout, this, [this](){
			if(_runCount == 0)
				deleteLater();
		});
		destroyTimer->start(500);
	}
}

void Client::close()
{
	QMetaObject::invokeMethod(_socket, "close", Qt::QueuedConnection);
}

void Client::sendMessage(const QByteArray &message)
{
	QMetaObject::invokeMethod(this, "doSend", Qt::QueuedConnection,
							  Q_ARG(QByteArray, message));
}

void Client::doSend(const QByteArray &message)
{
	_socket->sendBinaryMessage(message);
}

void Client::onRegister(const RegisterMessage &message)
{
	LOCK;
	if(_propHash.take("nonce").toULongLong() != message.nonce) {
		qWarning() << "Invalid nonce in register message";
		close();
		return;
	}

	_deviceId = _database->addNewDevice(message.deviceName,
										message.signAlgorithm,
										message.signKey,
										message.cryptAlgorithm,
										message.cryptKey);

	//TODO send reply
	qDebug() << "Created new device with id" << _deviceId;
}
