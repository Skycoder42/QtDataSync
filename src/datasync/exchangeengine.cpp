#include "exchangeengine_p.h"
#include "logger.h"

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
	_changeController(nullptr),
	_remoteConnector(nullptr)
{}

void ExchangeEngine::enterFatalState(const QString &error, const char *file, int line, const char *function, const char *category)
{
	QMessageLogContext context {file, line, function, category};
	_fatalErrorHandler(error, _defaults.setupName(), context);
}

ChangeController *ExchangeEngine::changeController() const
{
	return _changeController;
}

SyncManager::SyncState ExchangeEngine::state() const
{
	return _state;
}

void ExchangeEngine::initialize()
{
	try {
		_changeController = new ChangeController(_defaults, this);
		connect(_changeController, &ChangeController::changeTriggered,
				this, &ExchangeEngine::localDataChange);

		_remoteConnector = new RemoteConnector(_defaults, this);
		_remoteConnector->reconnect();
	} catch (Exception &e) {
		logFatal(e.qWhat());
	}
}

void ExchangeEngine::finalize()
{
	//TODO disconnect socket

	logDebug() << Q_FUNC_INFO;
	thread()->quit();
}

void ExchangeEngine::localDataChange()
{
	logDebug() << Q_FUNC_INFO;
}
