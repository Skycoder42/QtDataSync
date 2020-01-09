#ifndef QTDATASYNC_ENGINE_P_H
#define QTDATASYNC_ENGINE_P_H

#include "engine.h"
#include "authenticator.h"

#include "setup_p.h"
#include "databasewatcher_p.h"
#include "remoteconnector_p.h"

#ifndef QTDATASYNC_NO_NTP
#include "ntpsync_p.h"
#endif

#include <QtCore/QHash>
#include <QtCore/QQueue>
#include <QtCore/QLoggingCategory>

#include "enginestatemachine_p.h"
#include "tablestatemachine_p.h"
#include "tabledatamodel_p.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT EnginePrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(Engine)

public:
	using ErrorType = Engine::ErrorType;
	using ResyncFlag = Engine::ResyncFlag;
	using ResyncMode = Engine::ResyncMode;

	QScopedPointer<SetupPrivate> setup;

	RemoteConnector *connector = nullptr;
	EngineStateMachine *engineMachine = nullptr;
	QHash<QString, TableStateMachine*> tableMachines;

	QHash<QString, DatabaseWatcher*> watchers;

#ifndef QTDATASYNC_NO_NTP
	NtpSync *ntpSync = nullptr;
#endif

	static const SetupPrivate *setupFor(const Engine *engine);

	DatabaseWatcher *getWatcher(QSqlDatabase &&database);
	void dropWatcher(const QString &dbName);

	void setupConnections();
	void setupStateMachine();

	void onEnterError();
	void onEnterActive();
	void onEnterSignedIn();
	void onExitSignedIn();

	// common
	void _q_handleNetError(const QString &errorMessage);
	// authenticator
	void _q_signInSuccessful(const QString &userId, const QString &idToken);
	void _q_accountDeleted(bool success);
	// connector
	void _q_removedUser();
	// watchers
	void _q_tableAdded(const QString &name);
	void _q_tableRemoved(const QString &name);
};

Q_DECLARE_LOGGING_CATEGORY(logEngine)

}

#endif // QTDATASYNC_ENGINE_P_H
