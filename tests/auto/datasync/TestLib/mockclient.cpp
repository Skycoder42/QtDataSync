#include "mockclient.h"

#ifdef Q_OS_WIN
#define WAIT_TIMEOUT 15000
#else
#define WAIT_TIMEOUT 10000
#endif

MockClient::MockClient(QObject *parent) :
	MockConnection(new QWebSocket(), parent),
	_connectSpy(_socket, &QWebSocket::connected)
{}

bool MockClient::waitForConnected(quint16 port)
{
	QUrl url;
	url.setScheme(QStringLiteral("ws"));
	url.setHost(QHostAddress(QHostAddress::LocalHost).toString());
	url.setPort(port);
	_socket->open(url);

	auto ok = false;
	[&]() {
		if(_connectSpy.isEmpty())
			QVERIFY(_connectSpy.wait(WAIT_TIMEOUT));
		QVERIFY(!_connectSpy.isEmpty());
		ok = true;
	}();
	return ok;
}
