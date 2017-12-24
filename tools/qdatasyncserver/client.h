#ifndef CLIENT_H
#define CLIENT_H

#include <functional>

#include <QtCore/QJsonValue>
#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QThreadStorage>
#include <QtCore/QMutex>
#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>

#include <QtWebSockets/QWebSocket>

#include <cryptopp/osrng.h>

#include "databasecontroller.h"

#include "registermessage_p.h"
#include "loginmessage_p.h"
#include "syncmessage_p.h"
#include "changemessage_p.h"
#include "changedmessage_p.h"

class Client : public QObject
{
	Q_OBJECT

public:
	enum State {
		Authenticating,
		Idle,
		PushingChanges,
		PullingChanges,
		Error
	};
	Q_ENUM(State)

	explicit Client(DatabaseController *_database, QWebSocket *websocket, QObject *parent = nullptr);

public Q_SLOTS:
	void notifyChanged();

Q_SIGNALS:
	void connected(const QUuid &deviceId);

private Q_SLOTS:
	void binaryMessageReceived(const QByteArray &message);
	void error();
	void sslErrors(const QList<QSslError> &errors);
	void closeClient();
	void timeout();

private:
	static QThreadStorage<CryptoPP::AutoSeededRandomPool> rngPool;

	QByteArray _catStr;
	QScopedPointer<QLoggingCategory> _logCat;

	DatabaseController *_database;
	QWebSocket *_socket;
	QUuid _deviceId;

	QTimer *_idleTimer;

	QAtomicInt _runCount;

	//TODO check threadsafety on all methods? or enforce "single thread"
	QMutex _lock;
	State _state;
	QByteArray _loginNonce;
	quint64 _cachedChanges;
	QList<quint64> _activeDownloads;

	void run(const std::function<void()> &fn);
	const QLoggingCategory &logFn() const;

	void close();
	void closeLater();
	void sendMessage(const QByteArray &message);
	Q_INVOKABLE void doSend(const QByteArray &message);

	void onRegister(const QtDataSync::RegisterMessage &message, QDataStream &stream);
	void onLogin(const QtDataSync::LoginMessage &message, QDataStream &stream);
	void onSync(const QtDataSync::SyncMessage &message);
	void onChange(const QtDataSync::ChangeMessage &message);
	void onChangedAck(const QtDataSync::ChangedAckMessage &message);

	void triggerDownload(bool forceUpdate = false);
};

#endif // CLIENT_H
