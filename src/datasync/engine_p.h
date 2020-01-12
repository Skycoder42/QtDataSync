#ifndef QTDATASYNC_ENGINE_P_H
#define QTDATASYNC_ENGINE_P_H

#include "engine.h"
#include "authenticator.h"

#include "setup_impl.h"
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

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class EngineDataModel;
class TableDataModel;

class Q_DATASYNC_EXPORT EnginePrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(Engine)

public:
	using ErrorType = Engine::ErrorType;
	using ResyncFlag = Engine::ResyncFlag;
	using ResyncMode = Engine::ResyncMode;

	struct ErrorInfo {
		ErrorType type = ErrorType::Unknown;
		QString message;
		QVariant data;
	};

	QScopedPointer<SetupPrivate> setup;
	RemoteConnector *connector = nullptr;
	QHash<QString, DatabaseWatcher*> watchers;

	EngineStateMachine *engineMachine = nullptr;
	QPointer<EngineDataModel> engineModel;
	QHash<QString, std::pair<TableStateMachine*, QPointer<TableDataModel>>> tableMachines;
	QSet<QString> stopCache;

#ifndef QTDATASYNC_NO_NTP
	NtpSync *ntpSync = nullptr;
#endif

	static const SetupPrivate *setupFor(const Engine *engine);

	DatabaseWatcher *getWatcher(QSqlDatabase &&database);
	DatabaseWatcher *findWatcher(const QString &name);
	void dropWatcher(const QString &dbName);

	void setupStateMachine();

	// engine model
	void _q_startTableSync();
	void _q_stopTableSync();
	void _q_errorOccured(const ErrorInfo &info);
	// watchers
	void _q_tableAdded(const QString &table, bool liveSync);
	void _q_tableRemoved(const QString &table);
	// table model
	void _q_tableStopped(const QString &table);
	void _q_tableErrorOccured(const QString &table,
							  const ErrorInfo &info);
};

Q_DECLARE_LOGGING_CATEGORY(logEngine)

}

#endif // QTDATASYNC_ENGINE_P_H
