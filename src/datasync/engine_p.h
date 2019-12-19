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

#include "enginestatemachine.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT EnginePrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(Engine)

public:
	QScopedPointer<SetupPrivate> setup;

	QHash<QString, DatabaseWatcher*> dbWatchers;
	RemoteConnector *connector = nullptr;

	EngineStateMachine *statemachine = nullptr;
	QQueue<QString> dirtyTables;
	QString lastError;

#ifndef QTDATASYNC_NO_NTP
	NtpSync *ntpSync = nullptr;
#endif

	static const SetupPrivate *setupFor(const Engine *engine);

	void setupStateMachine();
	void setupConnector(const QString &userId);
	void fillDirtyTables();

	DatabaseWatcher *watcher(QSqlDatabase &&database);
	void removeWatcher(DatabaseWatcher *watcher);

	void onEnterActive();
	void onEnterDownloading();
	void onEnterUploading();

	void _q_handleError(const QString &errorMessage);
	void _q_signInSuccessful(const QString &userId, const QString &idToken);
	void _q_accountDeleted(bool success);
	void _q_triggerSync(const QString &type);
	void _q_syncDone(const QString &type);
	void _q_uploadedData(const ObjectKey &key);
};

Q_DECLARE_LOGGING_CATEGORY(logEngine)

}

#endif // QTDATASYNC_ENGINE_P_H
