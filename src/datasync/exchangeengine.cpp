#include "exchangeengine_p.h"
#include "logger.h"
#include "changecontroller_p.h"

#include <QtCore/QThread>

using namespace QtDataSync;

#define QTDATASYNC_LOG _logger

ExchangeEngine::ExchangeEngine(const QString &setupName) :
	QObject(),
	_state(SyncManager::Initializing),
	_defaults(setupName),
	_logger(_defaults.createLogger("engine", this)),
	_changeController(nullptr) //create early, to make accessible for localstores immediatly
{}

void ExchangeEngine::enterFatalState(const QString &error)
{
	logDebug() << Q_FUNC_INFO;
	Q_UNIMPLEMENTED();
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
	logDebug() << Q_FUNC_INFO;

	_changeController = new ChangeController(_defaults.setupName(), this);
}

void ExchangeEngine::finalize()
{
	logDebug() << Q_FUNC_INFO;
	Q_UNIMPLEMENTED();
	thread()->quit();
}
