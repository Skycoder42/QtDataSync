#ifndef QTDATASYNC_EXCHANGEENGINE_P_H
#define QTDATASYNC_EXCHANGEENGINE_P_H

#include <QtCore/QObject>
#include <QtCore/QAtomicPointer>
#include <QtCore/QThread>
#include <QtCore/QLockFile>

#include <QtRemoteObjects/QRemoteObjectHost>

#include "qtdatasync_global.h"
#include "syncmanager.h"
#include "defaults.h"
#include "setup.h"

#include "localstore_p.h"
#include "changecontroller_p.h"
#include "synccontroller_p.h"
#include "remoteconnector_p.h"

namespace QtDataSync {

class SyncManagerPrivate;
class AccountManagerPrivate;
class ChangeEmitter;

class Q_DATASYNC_EXPORT ExchangeEngine : public QObject
{
	Q_OBJECT

	Q_PROPERTY(SyncManager::SyncState state READ state NOTIFY stateChanged)
	Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
	Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
	struct Q_DATASYNC_EXPORT ImportData {
		QJsonObject data;
		QString password;
		bool keepData;
		bool allowFailure;

		bool isSet() const;

		// c++14 compat
		ImportData(QJsonObject data = {}, QString password = {}, bool keepData = false, bool allowFailure = false);
	};

	explicit ExchangeEngine(const QString &setupName,
							Setup::FatalErrorHandler errorHandler);
	~ExchangeEngine();

	Q_NORETURN void enterFatalState(const QString &error,
									const char *file,
									int line,
									const char *function,
									const char *category);

	Defaults defaults() const;
	ChangeController *changeController() const;
	RemoteConnector *remoteConnector() const;
	CryptoController *cryptoController() const;
	ChangeEmitter *emitter() const;

	SyncManager::SyncState state() const;
	qreal progress() const;
	QString lastError() const;

	void prepareInitialAccount(const ImportData &data);

public Q_SLOTS:
	void initialize();
	void finalize();

	void resetAccount(bool keepData, bool clearConfig = true);

Q_SIGNALS:
	void stateChanged(QtDataSync::SyncManager::SyncState state);
	void progressChanged(qreal progress);
	void lastErrorChanged(const QString &lastError);

private Q_SLOTS:
	void controllerError(const QString &errorMessage);
	void controllerTimeout();
	void remoteEvent(RemoteConnector::RemoteEvent event);
	void uploadingChanged(bool uploading);

	void addProgress(quint32 estimate);
	void incrementProgress();

private:
	SyncManager::SyncState _state = SyncManager::Initializing;
	quint32 _progressCurrent = 0;
	quint32 _progressMax = 0;
	QPointer<Controller> _progressAllowed;
	QString _lastError;

	Defaults _defaults;
	Logger *_logger;
	Setup::FatalErrorHandler _fatalErrorHandler;

	LocalStore *_localStore = nullptr;

	ChangeController *_changeController;
	SyncController *_syncController;
	RemoteConnector *_remoteConnector;

	QRemoteObjectHost *_roHost = nullptr;
	SyncManagerPrivate *_syncManager = nullptr;
	AccountManagerPrivate *_accountManager = nullptr;
	ChangeEmitter *_emitter;

	ImportData _initialImport;

	static Q_NORETURN void defaultFatalErrorHandler(const QString &error, const QString &setup, const QMessageLogContext &context);

	void connectController(Controller *controller);
	bool upstate(SyncManager::SyncState state);
	void clearError();
	void resetProgress(Controller *controller = nullptr);
};

class EngineThread : public QThread
{
	Q_OBJECT

public:
	EngineThread(QString setupName, ExchangeEngine *engine, QLockFile *lockFile);
	~EngineThread() override;

	QString name() const;
	ExchangeEngine *engine() const;

	bool startEngine();
	bool stopEngine();

	void waitAndTerminate(unsigned long timeout);

protected:
	void run() override;

private:
	const QString _name;
	QAtomicPointer<ExchangeEngine> _engine;
	QLockFile *_lockFile;
	QAtomicInteger<quint16> _running = false;

	//delete blocker
	QSharedPointer<EngineThread> _deleteBlocker;
};

}

#endif // QTDATASYNC_EXCHANGEENGINE_P_H
