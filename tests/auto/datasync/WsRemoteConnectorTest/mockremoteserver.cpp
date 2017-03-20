#include "mockremoteserver.h"
#include <QtTest>

MockRemoteServer::MockRemoteServer(QObject *parent) :
	QObject(parent),
	secret(QUuid::createUuid().toString()),
	expected(),
	reply(),
	server(new QWebSocketServer("MockRemoteServer", QWebSocketServer::NonSecureMode, this)),
	client(nullptr)
{
	connect(server, &QWebSocketServer::newConnection,
			this, &MockRemoteServer::newCon);
}

quint16 MockRemoteServer::setup()
{
	if(server->listen(QHostAddress::LocalHost))
		return server->serverPort();
	else
		return 0;
}

void MockRemoteServer::close()
{
	server->close();
}

void MockRemoteServer::newCon()
{
	QVERIFY(!client);
	client = server->nextPendingConnection();

	connect(client, &QWebSocket::disconnected,
			this, &MockRemoteServer::closeClient);
	connect(client, &QWebSocket::binaryMessageReceived,
			this, &MockRemoteServer::binaryMessageReceived);

	qDebug() << "client has connected!";
}

void MockRemoteServer::binaryMessageReceived(const QByteArray &message)
{
	qDebug() << "received message:" << message;
	QJsonParseError error;
	auto obj = QJsonDocument::fromJson(message, &error).object();
	QVERIFY2(error.error == QJsonParseError::NoError, error.errorString().toUtf8().constData());

	if(expected.isEmpty())
		QFAIL("Received unexpected message");
	auto next = expected.dequeue();
	foreach(auto key, next.keys())
		QCOMPARE(obj.value(key), next.value(key));

	if(!reply.isEmpty())
		client->sendBinaryMessage(QJsonDocument(reply.dequeue()).toJson());
}

void MockRemoteServer::closeClient()
{
	qDebug() << "client disconnected!";
	client->deleteLater();
	client = nullptr;
}
