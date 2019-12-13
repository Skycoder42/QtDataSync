#ifndef QTDATASYNC_ENGINE_P_H
#define QTDATASYNC_ENGINE_P_H

#include "engine.h"
#include "firebaseapibase_p.h"
#include "authenticator.h"
#include "setup_p.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT EnginePrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(Engine)
public:
	QScopedPointer<SetupPrivate> setup;

	static const SetupPrivate *setupFor(const Engine *engine);
};

}

#endif // QTDATASYNC_ENGINE_P_H
