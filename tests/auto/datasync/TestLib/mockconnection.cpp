#include "mockconnection.h"

#ifdef Q_OS_WIN
#define WAIT_TIMEOUT 15000
#else
#define WAIT_TIMEOUT 10000
#endif

MockConnection::MockConnection(QWebSocket *socket, QObject *parent) :
	QObject(parent),
	_socket(socket),
	_msgSpy(_socket, &QWebSocket::binaryMessageReceived),
	_closeSpy(_socket, &QWebSocket::disconnected)
{
	_socket->setParent(this);
	connect(_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, [this](QAbstractSocket::SocketError error) {
		if(_socket->error() == QAbstractSocket::RemoteHostClosedError ||
		   _socket->error() == QAbstractSocket::NetworkError ||
		   _socket->error() == QAbstractSocket::UnknownSocketError)
			return;
		QFAIL(qUtf8Printable(QString::number(error) + QStringLiteral(" - ") + _socket->errorString()));
	});
	connect(_socket, &QWebSocket::sslErrors, this, [this](const QList<QSslError> &errors) {
		for(auto error : errors)
			QFAIL(qUtf8Printable(error.errorString()));
	});
}

void MockConnection::sendBytes(const QByteArray &data)
{
	_socket->sendBinaryMessage(data);
}

void MockConnection::send(const QtDataSync::Message &message)
{
	_socket->sendBinaryMessage(message.serialize());
}

void MockConnection::sendSigned(const QtDataSync::Message &message, QtDataSync::ClientCrypto *crypto)
{
	_socket->sendBinaryMessage(message.serializeSigned(crypto->privateSignKey(), crypto->rng(), crypto));
}

void MockConnection::sendPing()
{
	_socket->sendBinaryMessage(QtDataSync::Message::PingMessage);
}

void MockConnection::close()
{
	_socket->close();
}

bool MockConnection::waitForNothing()
{
	return _msgSpy.isEmpty() &&
			!_msgSpy.wait() &&
			_closeSpy.isEmpty();
}

bool MockConnection::waitForPing()
{
	if(_hasPing) {
		_hasPing = false;
		return true;
	}

	auto ok = false;
	[&]() {
		if(_msgSpy.isEmpty())
			QVERIFY(_msgSpy.wait(60000 + WAIT_TIMEOUT)); // 1 min + WAIT_TIMEOUT
		QVERIFY(!_msgSpy.isEmpty());
		auto msg = _msgSpy.takeFirst()[0].toByteArray();
		ok = (msg == QtDataSync::Message::PingMessage);
	}();
	return ok;
}

bool MockConnection::waitForError(QtDataSync::ErrorMessage::ErrorType type, bool recoverable)
{
	return waitForReply<QtDataSync::ErrorMessage>([&](QtDataSync::ErrorMessage message, bool &ok) {
		QCOMPARE(message.type, type);
		QCOMPARE(message.canRecover, recoverable);
		ok = true;
	});
}

bool MockConnection::waitForDisconnect()
{
	auto ok = false;
	[&]() {
		if(_closeSpy.isEmpty())
			QVERIFY(_closeSpy.wait(WAIT_TIMEOUT));
		QVERIFY(!_closeSpy.isEmpty());
		ok = true;
	}();
	return ok;
}

bool MockConnection::waitForReplyImpl(const std::function<void(QByteArray, bool&)> &msgFn)
{
	auto ok = false;
	[&]() {
		QByteArray msg;
		do {
			if(_msgSpy.isEmpty())
				QVERIFY(_msgSpy.wait(WAIT_TIMEOUT));
			QVERIFY(!_msgSpy.isEmpty());
			msg = _msgSpy.takeFirst()[0].toByteArray();
			if(msg == QtDataSync::Message::PingMessage)
				_hasPing = true;
		} while(msg == QtDataSync::Message::PingMessage);
		try {
			msgFn(msg, ok);
		} catch (std::exception &e) {
			ok = false;
			QFAIL(e.what());
		}
	}();
	return ok;
}
