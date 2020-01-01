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
	using ErrorType = Engine::ErrorType;
	struct ErrorInfo {
		ErrorType type;
		QString message;
		QVariant data;
	};

	QScopedPointer<SetupPrivate> setup;

	EngineStateMachine *statemachine = nullptr;
	DatabaseProxy *dbProxy = nullptr;
	RemoteConnector *connector = nullptr;

	QQueue<CloudData> dlDataQueue;
	QHash<QString, QDateTime> dlLastSync;
	std::optional<ErrorInfo> lastError;

#ifndef QTDATASYNC_NO_NTP
	NtpSync *ntpSync = nullptr;
#endif

	static const SetupPrivate *setupFor(const Engine *engine);

	void setupConnections();
	void setupStateMachine();

	void onEnterError();
	void onEnterActive();
	void onEnterDownloading();
	void onEnterDlRunning();
	void onEnterProcRunning();
	void onEnterUploading();

	// global
	void _q_handleError(ErrorType type,
						const QString &errorMessage,
						const QVariant &errorData);
	void _q_handleNetError(const QString &errorMessage);
	// authenticator
	void _q_signInSuccessful(const QString &userId, const QString &idToken);
	void _q_accountDeleted(bool success);
	// dbwatcher
	void _q_triggerSync();
	// connector
	void _q_syncDone(const QString &type);
	void _q_downloadedData(const QList<CloudData> &data);
	void _q_uploadedData(const ObjectKey &key, const QDateTime &modified);
	// transformer
	void _q_transformDownloadDone(const LocalData &data);
};

Q_DECLARE_LOGGING_CATEGORY(logEngine)

}

#endif // QTDATASYNC_ENGINE_P_H
