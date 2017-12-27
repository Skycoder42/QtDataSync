#ifndef EXCHANGEENGINE_P_H
#define EXCHANGEENGINE_P_H

#include <QtCore/QObject>
#include <QtCore/QAtomicPointer>

#include "qtdatasync_global.h"
#include "syncmanager.h"
#include "defaults.h"
#include "setup.h"

#include "localstore_p.h"
#include "changecontroller_p.h"
#include "synccontroller_p.h"
#include "remoteconnector_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT SyncResultObject : public QObject
{
	Q_OBJECT
	friend class ExchangeEngine;

public:
	SyncResultObject(const std::function<void(SyncManager::SyncState)> &resFn, QObject *parent);

public Q_SLOTS:
	void triggerResult(QtDataSync::SyncManager::SyncState res);

private:
	bool _dOnly;
	std::function<void(SyncManager::SyncState)> _fn;
};

class Q_DATASYNC_EXPORT ExchangeEngine : public QObject
{
	Q_OBJECT

	Q_PROPERTY(SyncManager::SyncState state READ state NOTIFY stateChanged)
	Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
	typedef std::function<void(ExchangeEngine*)> RunFn;

	explicit ExchangeEngine(const QString &setupName,
							const Setup::FatalErrorHandler &errorHandler);

	Q_NORETURN void enterFatalState(const QString &error,
									const char *file,
									int line,
									const char *function,
									const char *category);

	ChangeController *changeController() const;
	RemoteConnector *remoteConnector() const;

	Q_INVOKABLE SyncManager::SyncState state() const;
	Q_INVOKABLE QString lastError() const;

public Q_SLOTS:
	void initialize();
	void finalize();

	void syncForResult(SyncResultObject *receiver, bool downloadOnly, bool triggerSync);

	//helper class initializer
	void runInitFunc(QtDataSync::ExchangeEngine::RunFn fn);

Q_SIGNALS:
	void stateChanged(QtDataSync::SyncManager::SyncState state);
	void lastErrorChanged(const QString &lastError);

private Q_SLOTS:
	void controllerError(const QString &errorMessage);
	void remoteEvent(RemoteConnector::RemoteEvent event);
	void uploadingChanged(bool uploading);

private:
	SyncManager::SyncState _state;
	QString _lastError;

	Defaults _defaults;
	Logger *_logger;
	Setup::FatalErrorHandler _fatalErrorHandler;

	LocalStore *_localStore;

	ChangeController *_changeController;
	SyncController *_syncController;
	RemoteConnector *_remoteConnector;

	static Q_NORETURN void defaultFatalErrorHandler(QString error, QString setup, const QMessageLogContext &context);

	void upstate(SyncManager::SyncState state);
	void clearError();
};

}

Q_DECLARE_METATYPE(QtDataSync::ExchangeEngine::RunFn)

#endif // EXCHANGEENGINE_P_H
