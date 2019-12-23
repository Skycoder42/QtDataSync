#ifndef QTDATASYNC_ENGINE_P_H
#define QTDATASYNC_ENGINE_P_H

#include "engine.h"
#include "authenticator.h"

#include "setup_p.h"
#include "databaseproxy_p.h"
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

	EngineStateMachine *statemachine = nullptr;
	DatabaseProxy *dbProxy = nullptr;
	RemoteConnector *connector = nullptr;

	QString lastError;

#ifndef QTDATASYNC_NO_NTP
	NtpSync *ntpSync = nullptr;
#endif

	static const SetupPrivate *setupFor(const Engine *engine);

	void setupStateMachine();
	void setupConnections();

	void onEnterActive();
	void onEnterDownloading();
	void onEnterUploading();

	// global
	void _q_handleError(const QString &errorMessage);
	// authenticator
	void _q_signInSuccessful(const QString &userId, const QString &idToken);
	void _q_accountDeleted(bool success);
	// dbwatcher
	void _q_triggerSync();
	// connector
	void _q_syncDone(const QString &type);
	void _q_uploadedData(const ObjectKey &key);
};

Q_DECLARE_LOGGING_CATEGORY(logEngine)

}

#endif // QTDATASYNC_ENGINE_P_H
