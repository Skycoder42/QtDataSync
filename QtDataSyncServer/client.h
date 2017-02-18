#ifndef CLIENT_H
#define CLIENT_H

#include "databasecontroller.h"

#include <QJsonValue>
#include <QObject>
#include <QUuid>
#include <QWebSocket>

class Client : public QObject
{
	Q_OBJECT

public:
	explicit Client(DatabaseController *database, QWebSocket *websocket, QObject *parent = nullptr);

	QUuid userId() const;

signals:
	void connected(const QUuid &deviceId, bool addClient);

private slots:
	void binaryMessageReceived(const QByteArray &message);
	void error();
	void sslErrors(const QList<QSslError> &errors);

private:
	DatabaseController *database;
	QWebSocket *socket;
	QUuid clientId;
	QUuid deviceId;

	QHostAddress socketAddress;

	void createIdentity(const QJsonObject &data);
	void identify(const QJsonObject &data);
	void save(const QJsonObject &data);

	void close();
	void sendCommand(const QByteArray &command, const QJsonValue &data = QJsonValue::Null);
	Q_INVOKABLE void doSend(const QByteArray &message);
};

#endif // CLIENT_H
