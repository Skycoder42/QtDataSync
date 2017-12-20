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
	_defaults(setupName),
	_logger(_defaults.createLogger("engine", this)),
	_fatalErrorHandler(errorHandler),
	_changeController(new ChangeController(_defaults, this)),
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

void ExchangeEngine::initialize()
{
	try {
		//change controller
		connect(_changeController, &ChangeController::uploadingChanged,
				this, &ExchangeEngine::uploadingChanged);

		//remote controller
		connect(_remoteConnector, &RemoteConnector::remoteEvent,
				this, &ExchangeEngine::remoteEvent);

		//initialize all
		_changeController->initialize();
		_remoteConnector->initialize();
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
		thread()->quit(); //TODO not threadsafe???
	});

	_remoteConnector->finalize();
	_changeController->finalize();
}

void ExchangeEngine::runInitFunc(const RunFn &fn)
{
	fn(this);
}

void ExchangeEngine::remoteEvent(RemoteConnector::RemoteEvent event)
{
	switch (event) {
	case RemoteConnector::RemoteDisconnected:
		upstate(SyncManager::Disconnected);
		_changeController->clearUploads();
		break;
	case RemoteConnector::RemoteConnecting:
		upstate(SyncManager::Initializing);
		_changeController->clearUploads();
		break;
	case RemoteConnector::RemoteReady: //TODO could also be error
		upstate(SyncManager::Uploading); //always assume uploading first, so no uploads can change to synced
		_changeController->setUploadingEnabled(true); //will emit "uploadingChanged" and thus change the thate
		break;
	case RemoteConnector::RemoteReadyWithChanges: //TODO always? could also be error
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
	if(uploading)
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
