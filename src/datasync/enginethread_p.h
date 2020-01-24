#ifndef QTDATASYNC_ENGINETHREAD_P_H
#define QTDATASYNC_ENGINETHREAD_P_H

#include "enginethread.h"

#include <QtCore/QAtomicPointer>

namespace QtDataSync {

class EngineThreadPrivate
{
public:
	struct Init {
		QScopedPointer<__private::SetupPrivate> setup;
		__private::SetupPrivate::ThreadInitFunc initFn;
	};
	std::optional<Init> init;

	QAtomicPointer<Engine> engine;
};

}

#endif // QTDATASYNC_ENGINETHREAD_P_H
