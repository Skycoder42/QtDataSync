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

#include "errormessage_p.h"
#include "registermessage_p.h"
#include "loginmessage_p.h"
#include "accessmessage_p.h"
#include "syncmessage_p.h"
#include "changemessage_p.h"
#include "changedmessage_p.h"
#include "devicesmessage_p.h"
#include "removemessage_p.h"
#include "proofmessage_p.h"
#include "devicechangemessage_p.h"
#include "macupdatemessage_p.h"
#include "keychangemessage_p.h"
#include "newkeymessage_p.h"

class Client : public QObject
{
	Q_OBJECT

public:
	enum State {
		Authenticating,
		AwatingGrant,
		Idle,
		Error
	};
	Q_ENUM(State)

	explicit Client(DatabaseController *_database, QWebSocket *websocket, QObject *parent = nullptr);

public Q_SLOTS:
	void dropConnection();
	void notifyChanged();
	void proofResult(bool success, const QtDataSync::AcceptMessage &message = {}); //empty key equals denied
	void sendProof(const QtDataSync::ProofMessage &message);
	void acceptDone(const QUuid &deviceId);

Q_SIGNALS:
	void connected(const QUuid &deviceId);
	void proofRequested(const QUuid &partner, const QtDataSync::ProofMessage &message);
	void proofDone(const QUuid &partner, bool success, const QtDataSync::AcceptMessage& message = {});
	void forceDisconnect(const QUuid &partner);

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
			return *d;
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
	quint32 _uploadLimit;
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
	//cached:
	QtDataSync::AccessMessage _cachedAccessRequest;
	QByteArray _cachedFingerPrint;

	void run(const std::function<void()> &fn);
	const QLoggingCategory &logFn() const;

	template<typename TMessage>
	void checkIdle(const TMessage & = {});

	void close();
	void closeLater();
	void sendMessage(const QtDataSync::Message &message);
	void sendError(const QtDataSync::ErrorMessage &message);
	Q_INVOKABLE void doSend(const QByteArray &message);

	void onRegister(const QtDataSync::RegisterMessage &message, QDataStream &stream);
	void onLogin(const QtDataSync::LoginMessage &message, QDataStream &stream);
	void onAccess(const QtDataSync::AccessMessage &message, QDataStream &stream);
	void onSync(const QtDataSync::SyncMessage &message);
	void onChange(const QtDataSync::ChangeMessage &message);
	void onDeviceChange(const QtDataSync::DeviceChangeMessage &message);
	void onChangedAck(const QtDataSync::ChangedAckMessage &message);
	void onListDevices(const QtDataSync::ListDevicesMessage &message);
	void onRemove(const QtDataSync::RemoveMessage &message);
	void onAccept(const QtDataSync::AcceptMessage &message, QDataStream &stream);
	void onDeny(const QtDataSync::DenyMessage &message);
	void onMacUpdate(const QtDataSync::MacUpdateMessage &message);
	void onKeyChange(const QtDataSync::KeyChangeMessage &message);
	void onNewKey(const QtDataSync::NewKeyMessage &message, QDataStream &stream);

	void triggerDownload(bool forceUpdate = false, bool skipNoChanges = false);
};

#endif // CLIENT_H
