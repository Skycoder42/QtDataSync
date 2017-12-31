#ifndef CLIENT_H
#define CLIENT_H

#include <functional>
#include <chrono>

#include <QtCore/QJsonValue>
#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QThreadStorage>
#include <QtCore/QMutex>
#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>

#include <QtWebSockets/QWebSocket>

#include <cryptopp/osrng.h>

#include "databasecontroller.h"
#include "singletaskqueue.h"

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
	//workaround because of alignment errors on msvc2015
	class Rng {
	public:
		inline Rng() :
			d(new CryptoPP::AutoSeededRandomPool())
		{}
		inline operator CryptoPP::RandomNumberGenerator&() {
			return *(d.data());
		}
	private:
		QScopedPointer<CryptoPP::AutoSeededRandomPool> d;
	};
	static QThreadStorage<Rng> rngPool;

	//logging stuff
	QByteArray _catStr;
	QScopedPointer<QLoggingCategory> _logCat;

	// "global" stuff
	DatabaseController *_database; //is threadsafe
	QWebSocket *_socket; //must only be accessed from the main thread

	// "constant" members, that wont change after the constructor
	QTimer *_idleTimer;
	quint32 _downLimit;
	quint32 _downThreshold;

	// thread safe task queue, ensures only 1 task per client is run at the same time
	SingleTaskQueue *_queue;

	//following members must only be accessed from within a task (to ensure thread safety)
	State _state;
	QUuid _deviceId;
	QByteArray _loginNonce;
	quint32 _cachedChanges;
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

	void triggerDownload(bool forceUpdate = false, bool skipNoChanges = false);
};

#endif // CLIENT_H
