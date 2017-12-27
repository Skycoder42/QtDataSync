#include "exchangeengine_p.h"
#include "logger.h"
#include "setup_p.h"

#include "changecontroller_p.h"
#include "remoteconnector_p.h"

#include <QtCore/QThread>

using namespace QtDataSync;

#define QTDATASYNC_LOG _logger

ExchangeEngine::ExchangeEngine(const QString &setupName, const Setup::FatalErrorHandler &errorHandler) :
	QObject(),
	_state(SyncManager::Initializing),
	_lastError(),
	_defaults(setupName),
	_logger(_defaults.createLogger("engine", this)),
	_fatalErrorHandler(errorHandler),
	_localStore(nullptr), //create in init thread!
	_changeController(new ChangeController(_defaults, this)),
	_syncController(new SyncController(_defaults, this)),
	_remoteConnector(new RemoteConnector(_defaults, this))
{}

void ExchangeEngine::enterFatalState(const QString &error, const char *file, int line, const char *function, const char *category)
{
	QMessageLogContext context {file, line, function, category};
	if(_fatalErrorHandler)
		_fatalErrorHandler(error, _defaults.setupName(), context);
	//fallback: in case the no handler or it returns
	defaultFatalErrorHandler(error, _defaults.setupName(), context);
}

ChangeController *ExchangeEngine::changeController() const
{
	return _changeController;
}

RemoteConnector *ExchangeEngine::remoteConnector() const
{
	return _remoteConnector;
}

SyncManager::SyncState ExchangeEngine::state() const
{
	return _state;
}

QString ExchangeEngine::lastError() const
{
	return _lastError;
}

void ExchangeEngine::initialize()
{
	try {
		_localStore = new LocalStore(_defaults.setupName(), this);

		//change controller
		connect(_changeController, &ChangeController::controllerError,
				this, &ExchangeEngine::controllerError);
		connect(_changeController, &ChangeController::uploadingChanged,
				this, &ExchangeEngine::uploadingChanged);
		connect(_changeController, &ChangeController::uploadChange,
				_remoteConnector, &RemoteConnector::uploadData);

		//sync controller
		connect(_syncController, &ChangeController::controllerError,
				this, &ExchangeEngine::controllerError);
		connect(_syncController, &SyncController::syncDone,
				_remoteConnector, &RemoteConnector::downloadDone);

		//remote controller
		connect(_remoteConnector, &RemoteConnector::controllerError,
				this, &ExchangeEngine::controllerError);
		connect(_remoteConnector, &RemoteConnector::remoteEvent,
				this, &ExchangeEngine::remoteEvent);
		connect(_remoteConnector, &RemoteConnector::uploadDone,
				_changeController, &ChangeController::uploadDone);
		connect(_remoteConnector, &RemoteConnector::downloadData,
				_syncController, &SyncController::syncChange);

		//initialize all
		QVariantHash params;
		params.insert(QStringLiteral("store"), QVariant::fromValue(_localStore));
		_changeController->initialize(params);
		_syncController->initialize(params);
		_remoteConnector->initialize(params);
		logDebug() << "Initialization completed";
	} catch (Exception &e) {
		logFatal(e.qWhat());
	} catch (std::exception &e) {
		logFatal(e.what());
	}
}

void ExchangeEngine::finalize()
{
	//TODO make generic for ALL controls classes
	connect(_remoteConnector, &RemoteConnector::finalized,
			this, [this](){
		logDebug() << "Finalization completed";
		thread()->quit();
	});

	_remoteConnector->finalize();
	_syncController->finalize();
	_changeController->finalize();
}

void ExchangeEngine::syncForResult(SyncResultObject *receiver, bool downloadOnly, bool triggerSync)
{
	receiver->_dOnly = downloadOnly;

	switch (_state) {
	case SyncManager::Error: //wont sync -> simply complete
	case SyncManager::Disconnected:
		QMetaObject::invokeMethod(receiver, "triggerResult",
								  Q_ARG(QtDataSync::SyncManager::SyncState, _state));
		break;
	case SyncManager::Uploading: //if download only -> done
		if(downloadOnly) {
			QMetaObject::invokeMethod(receiver, "triggerResult",
									  Q_ARG(QtDataSync::SyncManager::SyncState, _state));
			break;
		}
		Q_FALLTHROUGH();
	case SyncManager::Synchronized: //if wants sync -> trigger it, then...
		if(triggerSync)
			_remoteConnector->resync();
		Q_FALLTHROUGH();
	case SyncManager::Initializing: //conntect to react to result
	case SyncManager::Downloading:
		connect(this, &ExchangeEngine::stateChanged,
				receiver, &SyncResultObject::triggerResult);
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void ExchangeEngine::runInitFunc(RunFn fn)
{
	fn(this);
}

void ExchangeEngine::controllerError(const QString &errorMessage)
{
	_lastError = errorMessage;
	emit lastErrorChanged(errorMessage); //first error, that state (so someone that reacts to states can read the error)
	upstate(SyncManager::Error);

	//stop up/downloading etc.
	_remoteConnector->disconnect();
	_changeController->clearUploads();
}

void ExchangeEngine::remoteEvent(RemoteConnector::RemoteEvent event)
{
	if(_state == SyncManager::Error &&
	   event != RemoteConnector::RemoteConnecting) //only a reconnect event can get the system out of the error state
		return;

	switch (event) {
	case RemoteConnector::RemoteDisconnected:
		upstate(SyncManager::Disconnected);
		_changeController->clearUploads();
		break;
	case RemoteConnector::RemoteConnecting:
		upstate(SyncManager::Initializing);
		clearError();
		_changeController->clearUploads();
		break;
	case RemoteConnector::RemoteReady:
		upstate(SyncManager::Uploading); //always assume uploading first, so no uploads can change to synced
		_changeController->setUploadingEnabled(true); //will emit "uploadingChanged" and thus change the thate
		break;
	case RemoteConnector::RemoteReadyWithChanges:
		upstate(SyncManager::Downloading);
		_changeController->setUploadingEnabled(false);
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

void ExchangeEngine::uploadingChanged(bool uploading)
{
	if(_state == SyncManager::Error)
		return;
	else if(uploading)
		upstate(SyncManager::Uploading);
	else if(_state == SyncManager::Uploading)
		upstate(SyncManager::Synchronized);
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

void ExchangeEngine::upstate(SyncManager::SyncState state)
{
	if(_state != state) {
		_state = state;
		emit stateChanged(state);
	}
}

void ExchangeEngine::clearError()
{
	if(!_lastError.isNull()) {
		_lastError.clear();
		emit lastErrorChanged(QString());
	}
}



SyncResultObject::SyncResultObject(const std::function<void (SyncManager::SyncState)> &resFn, QObject *parent) :
	QObject(parent),
	_dOnly(false),
	_fn(resFn)
{}

void SyncResultObject::triggerResult(SyncManager::SyncState res)
{
	switch (res) {
	case SyncManager::Initializing: // do nothing
	case SyncManager::Downloading:
		break;
	case SyncManager::Uploading: // download only -> done, else do nothing
		if(!_dOnly)
			break;
		Q_FALLTHROUGH();
	case SyncManager::Synchronized: //done
	case SyncManager::Error:
	case SyncManager::Disconnected:
		_fn(res);
		deleteLater();
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}
