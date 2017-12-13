#ifndef CLIENT_H
#define CLIENT_H

#include <QtCore/QJsonValue>
#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QThreadStorage>
#include <QtCore/QMutex>
#include <QtCore/QHash>

#include <QtWebSockets/QWebSocket>

#include <cryptopp/osrng.h>

#include "databasecontroller.h"

#include "registermessage_p.h"

class Client : public QObject
{
	Q_OBJECT

public:
	explicit Client(DatabaseController *_database, QWebSocket *websocket, QObject *parent = nullptr);

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
	static QThreadStorage<CryptoPP::AutoSeededRandomPool> rngPool;

	DatabaseController *_database;
	QWebSocket *_socket;
	QUuid _deviceId;

	QAtomicInt _runCount;

	QMutex _propMutex;
	QHash<QByteArray, QVariant> _propHash;

	void close();
	void sendMessage(const QByteArray &message);
	Q_INVOKABLE void doSend(const QByteArray &message);

	void onRegister(const QtDataSync::RegisterMessage &message);
};

#endif // CLIENT_H
