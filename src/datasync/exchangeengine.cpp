#include "exchangeengine_p.h"
#include "logger.h"

#include "changecontroller_p.h"
#include "remoteconnector_p.h"

#include <QtCore/QThread>

using namespace QtDataSync;

#define QTDATASYNC_LOG _logger

ExchangeEngine::ExchangeEngine(const QString &setupName) :
	QObject(),
	_state(SyncManager::Initializing),
	_defaults(setupName),
	_logger(_defaults.createLogger("engine", this)),
	_changeController(nullptr),
	_remoteConnector(nullptr)
{}

void ExchangeEngine::enterFatalState(const QString &error)
{
	logDebug() << Q_FUNC_INFO;
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
	_changeController = new ChangeController(_defaults, this);
	connect(_changeController, &ChangeController::changeTriggered,
			this, &ExchangeEngine::localDataChange);

	_remoteConnector = new RemoteConnector(_defaults, this);
}

void ExchangeEngine::finalize()
{
	logDebug() << Q_FUNC_INFO;
	thread()->quit();
}

void ExchangeEngine::localDataChange()
{
	logDebug() << Q_FUNC_INFO;
}
