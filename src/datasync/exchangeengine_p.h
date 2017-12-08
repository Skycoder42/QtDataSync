#ifndef EXCHANGEENGINE_P_H
#define EXCHANGEENGINE_P_H

#include <QtCore/QObject>
#include <QtCore/QAtomicPointer>

#include "qtdatasync_global.h"
#include "syncmanager.h"
#include "defaults.h"
#include "setup.h"

namespace QtDataSync {

class ChangeController;
class RemoteConnector;

class Q_DATASYNC_EXPORT ExchangeEngine : public QObject
{
	Q_OBJECT

	Q_PROPERTY(SyncManager::SyncState state READ state NOTIFY stateChanged)

public:
	explicit ExchangeEngine(const QString &setupName,
							const Setup::FatalErrorHandler &errorHandler);

	Q_NORETURN void enterFatalState(const QString &error,
									const char *file,
									int line,
									const char *function,
									const char *category);

	ChangeController *changeController() const;

	SyncManager::SyncState state() const;

public Q_SLOTS:
	void initialize();
	void finalize();

Q_SIGNALS:
	void stateChanged(SyncManager::SyncState state);

private Q_SLOTS:
	void localDataChange();

private:
	SyncManager::SyncState _state;

	Defaults _defaults;
	Logger *_logger;
	Setup::FatalErrorHandler _fatalErrorHandler;

	QAtomicPointer<ChangeController> _changeController;
	RemoteConnector *_remoteConnector;

	static Q_NORETURN void defaultFatalErrorHandler(QString error, QString setup, const QMessageLogContext &context);
};

}

#endif // EXCHANGEENGINE_P_H
