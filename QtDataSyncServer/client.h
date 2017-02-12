#ifndef CLIENT_H
#define CLIENT_H

#include <QJsonValue>
#include <QObject>
#include <QUuid>
#include <QWebSocket>

class Client : public QObject
{
	Q_OBJECT

public:
	explicit Client(QWebSocket *websocket, QObject *parent = nullptr);

	QUuid userId() const;

signals:
	void connected(const QUuid &deviceId, bool addClient);

private slots:
	void binaryMessageReceived(const QByteArray &message);
	void error();
	void sslErrors(const QList<QSslError> &errors);

private:
	QWebSocket *socket;
	QUuid clientId;

	void sendCommand(const QByteArray &command, const QJsonValue &data = QJsonValue::Null);
};

#endif // CLIENT_H
