#ifndef CLIENT_H
#define CLIENT_H

#include <QtCore/QJsonValue>
#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QThreadStorage>
#include <QtCore/QMutex>
#include <QtCore/QHash>
#include <QtCore/QTimer>

#include <QtWebSockets/QWebSocket>

#include <cryptopp/osrng.h>

#include "databasecontroller.h"

#include "registermessage_p.h"
#include "loginmessage_p.h"

class ClientException : public QException
{
public:
	ClientException(const QByteArray &what);

	const char *what() const noexcept override;
	void raise() const override;
	QException *clone() const override;

private:
	const QByteArray _what;
};

class Client : public QObject
{
	Q_OBJECT

public:
	enum State {
		Authenticating,
		Idle,
		Error
	};
	Q_ENUM(State)

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
	void timeout();

private:
	static const QByteArray PingMessage;
	static QThreadStorage<CryptoPP::AutoSeededRandomPool> rngPool;

	DatabaseController *_database;
	QWebSocket *_socket;
	QUuid _deviceId;

	QTimer *_idleTimer;

	QAtomicInt _runCount;

	QMutex _lock;
	State _state;
	QHash<QByteArray, QVariant> _properties;

	void close();
	void sendMessage(const QByteArray &message);
	Q_INVOKABLE void doSend(const QByteArray &message);

	void onRegister(const QtDataSync::RegisterMessage &message, QDataStream &stream);
	void onLogin(const QtDataSync::LoginMessage &message, QDataStream &stream);
};

#endif // CLIENT_H
