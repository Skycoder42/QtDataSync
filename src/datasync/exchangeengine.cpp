#include "exchangeengine_p.h"
#include "logger.h"
#include "setup_p.h"
#include "defaults_p.h"

#include "accountmanager_p.h"
#include "changecontroller_p.h"
#include "remoteconnector_p.h"
#include "syncmanager_p.h"
#include "changeemitter_p.h"

#include <QtCore/QThread>

using namespace QtDataSync;

#define QTDATASYNC_LOG _logger

ExchangeEngine::ExchangeEngine(const QString &setupName, const Setup::FatalErrorHandler &errorHandler) :
	QObject(),
	_state(SyncManager::Initializing),
	_progressCurrent(0),
	_progressMax(0),
	_progressAllowed(nullptr),
	_lastError(),
	_defaults(DefaultsPrivate::obtainDefaults(setupName)),
	_logger(_defaults.createLogger("engine", this)),
	_fatalErrorHandler(errorHandler),
	_localStore(nullptr), //create in init thread!
	_changeController(new ChangeController(_defaults, this)),
	_syncController(new SyncController(_defaults, this)),
	_remoteConnector(new RemoteConnector(_defaults, this)),
	_roHost(nullptr),
	_syncManager(nullptr),
	_accountManager(nullptr),
	_emitter(new ChangeEmitter(_defaults, this)) //must be created here, because of access
{}

void ExchangeEngine::enterFatalState(const QString &error, const char *file, int line, const char *function, const char *category)
{
	QMessageLogContext context {file, line, function, category};
	if(_fatalErrorHandler)
		_fatalErrorHandler(error, _defaults.setupName(), context);
	//fallback: in case the no handler or it returns
	defaultFatalErrorHandler(error, _defaults.setupName(), context);
}

Defaults ExchangeEngine::defaults() const
{
	return _defaults;
}

ChangeController *ExchangeEngine::changeController() const
{
	return _changeController;
}

RemoteConnector *ExchangeEngine::remoteConnector() const
{
	return _remoteConnector;
}

CryptoController *ExchangeEngine::cryptoController() const
{
	return _remoteConnector->cryptoController();
}

ChangeEmitter *ExchangeEngine::emitter() const
{
	return _emitter;
}

SyncManager::SyncState ExchangeEngine::state() const
{
	return _state;
}

qreal ExchangeEngine::progress() const
{
	if(_progressMax == 0)
		return -1.0;
	else
		return static_cast<qreal>(_progressCurrent)/static_cast<qreal>(_progressMax);
}

QString ExchangeEngine::lastError() const
{
	return _lastError;
}

void ExchangeEngine::initialize()
{
	logDebug() << "Beginning engine initialization";
	try {
		_localStore = new LocalStore(_defaults, this);

		//change controller
		connectController(_changeController);
		connect(_changeController, &ChangeController::uploadingChanged,
				this, &ExchangeEngine::uploadingChanged);
		connect(_changeController, &ChangeController::uploadChange,
				_remoteConnector, &RemoteConnector::uploadData);
		connect(_changeController, &ChangeController::uploadDeviceChange,
				_remoteConnector, &RemoteConnector::uploadDeviceData);

		//sync controller
		connectController(_syncController);
		connect(_syncController, &SyncController::syncDone,
				_remoteConnector, &RemoteConnector::downloadDone);

		//remote controller
		connectController(_remoteConnector);
		connect(_remoteConnector, &RemoteConnector::remoteEvent,
				this, &ExchangeEngine::remoteEvent);
		connect(_remoteConnector, &RemoteConnector::updateUploadLimit,
				_changeController, &ChangeController::updateUploadLimit);
		connect(_remoteConnector, &RemoteConnector::uploadDone,
				_changeController, &ChangeController::uploadDone);
		connect(_remoteConnector, &RemoteConnector::deviceUploadDone,
				_changeController, &ChangeController::deviceUploadDone);
		connect(_remoteConnector, &RemoteConnector::downloadData,
				_syncController, &SyncController::syncChange);
		connect(_remoteConnector, &RemoteConnector::accountAccessGranted,
				_localStore, &LocalStore::prepareAccountAdded);

		//initialize all
		QVariantHash params;
		params.insert(QStringLiteral("store"), QVariant::fromValue(_localStore));
		params.insert(QStringLiteral("emitter"), QVariant::fromValue(_emitter));
		_changeController->initialize(params);
		_syncController->initialize(params);
		_remoteConnector->initialize(params);
		logDebug() << "Controller initialization completed";

		//create remote object stuff
		_roHost = new QRemoteObjectHost(_defaults.remoteAddress(), this);
		_roHost->enableRemoting(_emitter);
		_syncManager = new SyncManagerPrivate(this);
		_roHost->enableRemoting(_syncManager);
		_accountManager = new AccountManagerPrivate(this);
		_roHost->enableRemoting(_accountManager);
		logDebug() << "RemoteObject host node initialized";
	} catch (Exception &e) {
		logFatal(e.qWhat());
	} catch (std::exception &e) {
		logFatal(e.what());
	}
}

void ExchangeEngine::finalize()
{
	logDebug() << "Beginning engine finalization";

	//remoteconnector is the only one asynchronous (for now)
	connect(_remoteConnector, &RemoteConnector::finalized,
			this, [this](){
		logDebug() << "Finalization completed";
		thread()->quit();
	});

	_syncController->finalize();
	_changeController->finalize();
	_remoteConnector->finalize();
}

void ExchangeEngine::resetAccount(bool keepData, bool clearConfig)
{
	logDebug() << "Resetting local account. Keep local data:" << keepData;
	_syncController->setSyncEnabled(false);
	_changeController->clearUploads();
	_localStore->reset(keepData);
	_remoteConnector->resetAccount(clearConfig);
}

void ExchangeEngine::controllerError(const QString &errorMessage)
{
	_lastError = errorMessage;
	emit lastErrorChanged(errorMessage); //first error, than state (so someone that reacts to states can read the error)
	upstate(SyncManager::Error);

	//stop up/downloading etc.
	_remoteConnector->disconnectRemote();
	_changeController->clearUploads();
}

void ExchangeEngine::controllerTimeout()
{
	logWarning() << "Internal operation timeout from" << QObject::sender()->metaObject()->className()
				 << "- reconnecting to remote";
	_remoteConnector->reconnect();
}

void ExchangeEngine::remoteEvent(RemoteConnector::RemoteEvent event)
{
	if(_state == SyncManager::Error &&
	   event != RemoteConnector::RemoteConnecting) //only a reconnect event can get the system out of the error state
		return;

	logDebug() << "Handling remote event:" << event;
	switch (event) {
	case RemoteConnector::RemoteDisconnected:
		upstate(SyncManager::Disconnected);
		resetProgress();
		_syncController->setSyncEnabled(false);
		_changeController->clearUploads();
		break;
	case RemoteConnector::RemoteConnecting:
		upstate(SyncManager::Initializing);
		resetProgress();
		clearError();
		_syncController->setSyncEnabled(false);
		_changeController->clearUploads();
		break;
	case RemoteConnector::RemoteReady:
		upstate(SyncManager::Uploading); //always assume uploading first, so no uploads can change to synced
		resetProgress(_changeController); //allows ccon changes
		_syncController->setSyncEnabled(true);
		_changeController->setUploadingEnabled(true); //will emit "uploadingChanged" and thus change the thate
		break;
	case RemoteConnector::RemoteReadyWithChanges:
		if(upstate(SyncManager::Downloading))
			resetProgress(_remoteConnector); //allows rcon changes
		_syncController->setSyncEnabled(true);
		_changeController->setUploadingEnabled(false);//disable uploads, but allow finishing
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void ExchangeEngine::uploadingChanged(bool uploading)
{
	logDebug() << "Uploading state change to:" << uploading;
	if(_state == SyncManager::Error)
		return;
	else if(uploading) {
		if(upstate(SyncManager::Uploading))
			resetProgress(_changeController); //allows ccon changes
	} else if(_state == SyncManager::Uploading) {
		upstate(SyncManager::Synchronized);
		resetProgress();
	}
}

void ExchangeEngine::addProgress(quint32 estimate)
{
	if(sender() == _progressAllowed) {
		_progressMax += estimate;
		emit progressChanged(progress());
	}
}

void ExchangeEngine::incrementProgress()
{
	if(sender() == _progressAllowed) {
		_progressCurrent++;
		emit progressChanged(progress());
	}
}

void ExchangeEngine::defaultFatalErrorHandler(QString error, QString setup, const QMessageLogContext &context)
{
	auto name = setup.toUtf8();
	auto msg = error.toUtf8();
	QMessageLogger(context.file, context.line, context.function, context.category)
			.fatal("Unrecoverable error for \"%s\" - killing application. Error Message:\n\t%s",
				   name.constData(),
				   msg.constData());
}

void ExchangeEngine::connectController(Controller *controller)
{
	connect(controller, &Controller::controllerError,
			this, &ExchangeEngine::controllerError);
	connect(controller, &Controller::operationTimeout,
			this, &ExchangeEngine::controllerTimeout);
	connect(controller, &Controller::progressAdded,
			this, &ExchangeEngine::addProgress);
	connect(controller, &Controller::progressIncrement,
			this, &ExchangeEngine::incrementProgress);
}

bool ExchangeEngine::upstate(SyncManager::SyncState state)
{
	if(_state != state) {
		_state = state;
		emit stateChanged(state);
		return true;
	} else
		return false;
}

void ExchangeEngine::clearError()
{
	if(!_lastError.isNull()) {
		_lastError.clear();
		emit lastErrorChanged(QString());
	}
}

void ExchangeEngine::resetProgress(Controller *controller)
{
	const auto changed = (_progressMax != 0);
	_progressCurrent = 0;
	_progressMax = 0;
	if(changed)
		emit progressChanged(progress());
	_progressAllowed = controller;
}
