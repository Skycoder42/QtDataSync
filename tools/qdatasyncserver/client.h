#ifndef CLIENT_H
#define CLIENT_H

#include <QtCore/QJsonValue>
#include <QtCore/QObject>
#include <QtCore/QUuid>

#include <QtWebSockets/QWebSocket>

#include "databasecontroller.h"

#include "registermessage_p.h"

class Client : public QObject
{
	Q_OBJECT

public:
	explicit Client(DatabaseController *database, QWebSocket *websocket, QObject *parent = nullptr);

	QUuid deviceId() const;

public slots:
	void notifyChanged(const QString &type,
					   const QString &key,
					   bool changed);

signals:
	void connected(const QUuid &userId, bool addClient);

private slots:
	void binaryMessageReceived(const QByteArray &message);
	void error();
	void sslErrors(const QList<QSslError> &errors);
	void closeClient();

private:
	DatabaseController *database;
	QWebSocket *socket;
	QUuid userId;
	QUuid devId;

	QHostAddress socketAddress;
	QAtomicInt runCount;

	void close();
	void sendMessage(const QByteArray &message);
	Q_INVOKABLE void doSend(const QByteArray &message);

	void onRegister(const QtDataSync::RegisterMessage &message);
};

#endif // CLIENT_H
