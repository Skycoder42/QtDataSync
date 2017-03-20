#ifndef MOCKREMOTESERVER_H
#define MOCKREMOTESERVER_H

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QQueue>
#include <QWebSocket>
#include <QWebSocketServer>

class MockRemoteServer : public QObject
{
	Q_OBJECT

public:
	explicit MockRemoteServer(QObject *parent = nullptr);

	quint16 setup();
	void close();

private slots:
	void newCon();
	void binaryMessageReceived(const QByteArray &message);
	void closeClient();

public:
	QString secret;
	QQueue<QHash<QString, QJsonValue>> expected;
	QQueue<QJsonObject> reply;

private:
	QWebSocketServer *server;
	QWebSocket *client;
};

#endif // MOCKREMOTESERVER_H
