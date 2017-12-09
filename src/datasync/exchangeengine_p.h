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
	RemoteConnector *remoteConnector() const;

	SyncManager::SyncState state() const;

public Q_SLOTS:
	void initialize();
	void finalize();

Q_SIGNALS:
	void stateChanged(SyncManager::SyncState state);

private Q_SLOTS:
	void localDataChange();

private:
	template <typename T>
	class AtomicPointer : public QAtomicPointer<T>
	{
	public:
		inline AtomicPointer(T *ptr = nullptr);
		inline T *operator->() const;
	};

	SyncManager::SyncState _state;

	Defaults _defaults;
	Logger *_logger;
	Setup::FatalErrorHandler _fatalErrorHandler;

	AtomicPointer<ChangeController> _changeController;
	AtomicPointer<RemoteConnector> _remoteConnector;

	static Q_NORETURN void defaultFatalErrorHandler(QString error, QString setup, const QMessageLogContext &context);
};

// ------------- Generic Implementation -------------

template<typename T>
inline ExchangeEngine::AtomicPointer<T>::AtomicPointer(T *ptr) :
	QAtomicPointer<T>(ptr)
{}

template<typename T>
inline T *ExchangeEngine::AtomicPointer<T>::operator->() const
{
	return this->loadAcquire();
}

}

#endif // EXCHANGEENGINE_P_H
