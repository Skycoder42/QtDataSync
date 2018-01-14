#include "mockserver.h"

MockServer::MockServer(QObject *parent) :
	QObject(parent),
	_server(new QWebSocketServer(QStringLiteral("mockserver"), QWebSocketServer::NonSecureMode, this)),
	_connectedSpy(_server, &QWebSocketServer::newConnection)
{
	connect(_server, &QWebSocketServer::acceptError, this, [this]() {
		QFAIL(qUtf8Printable(_server->errorString()));
	});
	connect(_server, &QWebSocketServer::serverError, this, [this]() {
		QFAIL(qUtf8Printable(_server->errorString()));
	});
	connect(_server, &QWebSocketServer::closed, this, [this]() {
		QFAIL(qUtf8Printable(_server->errorString()));
	});
	connect(_server, &QWebSocketServer::peerVerifyError, this, [this](const QSslError &error) {
		QFAIL(qUtf8Printable(error.errorString()));
	});
	connect(_server, &QWebSocketServer::sslErrors, this, [this](const QList<QSslError> &errors) {
		foreach(auto error, errors)
			QFAIL(qUtf8Printable(error.errorString()));
	});
}

void MockServer::init()
{
	QVERIFY(_server->listen(QHostAddress::LocalHost));
}

QUrl MockServer::url() const
{
	return _server->serverUrl();
}

void MockServer::clear()
{
	_connectedSpy.clear();
}

bool MockServer::waitForConnected(MockConnection **connection, int timeout)
{
	auto ok = false;
	[&]() {
		QVERIFY(connection);
		if(_connectedSpy.isEmpty())
			QVERIFY(_connectedSpy.wait(timeout));
		QVERIFY(!_connectedSpy.isEmpty());
		QVERIFY(_server->hasPendingConnections());
		_connectedSpy.removeFirst();
		*connection = new MockConnection(_server->nextPendingConnection(), this);
		ok = true;
	}();
	return ok;
}



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
		foreach(auto error, errors)
			QFAIL(qUtf8Printable(error.errorString()));
	});
}

void MockConnection::send(const QtDataSync::Message &message)
{
	_socket->sendBinaryMessage(message.serialize());
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
	return !_msgSpy.wait();
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
			QVERIFY(_msgSpy.wait(65000)); // 1 min 5 secs
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
			QVERIFY(_closeSpy.wait());
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
				QVERIFY(_msgSpy.wait());
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
