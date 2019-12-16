#ifndef QTDATASYNC_ENGINE_P_H
#define QTDATASYNC_ENGINE_P_H

#include "engine.h"
#include "authenticator.h"

#include "firebaseapibase_p.h"
#include "databasewatcher_p.h"
#include "setup_p.h"

#include <QtCore/QHash>

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT EnginePrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(Engine)

public:
	QScopedPointer<SetupPrivate> setup;

	QHash<QString, DatabaseWatcher*> dbWatchers;

	static const SetupPrivate *setupFor(const Engine *engine);
};

}

#endif // QTDATASYNC_ENGINE_P_H
