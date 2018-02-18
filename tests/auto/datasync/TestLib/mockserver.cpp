#include "mockserver.h"

#ifdef Q_OS_WIN
#define WAIT_TIMEOUT 10000
#else
#define WAIT_TIMEOUT 5000
#endif

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
		for(auto error : errors)
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
			QVERIFY(_connectedSpy.wait(WAIT_TIMEOUT + timeout));
		QVERIFY(!_connectedSpy.isEmpty());
		QVERIFY(_server->hasPendingConnections());
		_connectedSpy.removeFirst();
		*connection = new MockConnection(_server->nextPendingConnection(), this);
		ok = true;
	}();
	return ok;
}
