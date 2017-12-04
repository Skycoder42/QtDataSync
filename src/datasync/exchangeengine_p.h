#ifndef EXCHANGEENGINE_P_H
#define EXCHANGEENGINE_P_H

#include <QtCore/QObject>
#include <QtCore/QAtomicPointer>

#include "qtdatasync_global.h"
#include "syncmanager.h"
#include "defaults.h"

namespace QtDataSync {

class ChangeController;
class RemoteConnector;

class Q_DATASYNC_EXPORT ExchangeEngine : public QObject
{
	Q_OBJECT

	Q_PROPERTY(SyncManager::SyncState state READ state NOTIFY stateChanged)

public:
	explicit ExchangeEngine(const QString &setupName);

	void enterFatalState(const QString &error);

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

	QAtomicPointer<ChangeController> _changeController;
	RemoteConnector *_remoteConnector;
};

}

#endif // EXCHANGEENGINE_P_H
