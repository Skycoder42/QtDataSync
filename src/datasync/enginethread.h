#ifndef QTDATASYNC_ENGINETHREAD_H
#define QTDATASYNC_ENGINETHREAD_H

#include "QtDataSync/qtdatasync_global.h"
#include "engine.h"
#include "setup_impl.h"

#include <QtCore/qthread.h>
#include <QtCore/qfuture.h>

namespace QtDataSync {

class AsyncWatcher;

class EngineThreadPrivate;
class Q_DATASYNC_EXPORT EngineThread : public QThread
{
	Q_OBJECT

public:
	~EngineThread() override;

	Engine *engine() const;
	QFuture<AsyncWatcher*> createWatcher(QObject *parent = nullptr) const;

Q_SIGNALS:
	void engineCreated(Engine *engine, QPrivateSignal = {});

protected:
	void run() override;

private:
	friend class __private::SetupPrivate;
	QScopedPointer<EngineThreadPrivate> d;

	explicit EngineThread(QScopedPointer<__private::SetupPrivate> &&setup,
						  __private::SetupPrivate::ThreadInitFunc &&initFn,
						  QObject *parent = nullptr);
};

}

#endif // QTDATASYNC_ENGINETHREAD_H
