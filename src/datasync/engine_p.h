#ifndef QTDATASYNC_ENGINE_P_H
#define QTDATASYNC_ENGINE_P_H

#include "engine.h"
#include "authenticator.h"

#include "setup_impl.h"
#include "firebaseauthenticator_p.h"
#include "databasewatcher_p.h"
#include "remoteconnector_p.h"
#include "settingsadaptor_p.h"

#ifndef QTDATASYNC_NO_NTP
#include "ntpsync_p.h"
#endif

#include <QtCore/QHash>
#include <QtCore/QQueue>
#include <QtCore/QLoggingCategory>
#include <QtCore/QAtomicInteger>
#include <QtCore/QMutex>

#include "enginestatemachine_p.h"
#include "tablestatemachine_p.h"
#include "rep_asyncwatcher_merged.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class EngineDataModel;
class TableDataModel;

class AsyncWatcherBackend final : public AsyncWatcherSource
{
	Q_OBJECT

public:
	AsyncWatcherBackend(Engine *engine);

	Q_INVOKABLE QList<TableEvent> activeTables() const final;

public Q_SLOTS:
	void activate(const QString &name) final;
};

class TableConfigData : public QSharedData
{
public:
	using Reference = TableConfig::Reference;

	QString table;
	QSqlDatabase connection;

	QVersionNumber version;
	bool liveSyncEnabled = false;
	bool forceCreate = false;
	QString primaryKey;
	QStringList fields;
	QList<Reference> references;

	inline TableConfigData(QString &&table, QSqlDatabase &&connection) :
		table{std::move(table)},
		connection{std::move(connection)}
	{}
};

class Q_DATASYNC_EXPORT EnginePrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(Engine)

public:
	using ErrorType = Engine::ErrorType;
	using ResyncFlag = Engine::ResyncFlag;
	using ResyncMode = Engine::ResyncMode;

	struct ErrorInfo {
		ErrorType type = ErrorType::Unknown;
		QVariant data;
	};

	struct TableInfo {
		TableStateMachine *machine = nullptr;
		QPointer<TableDataModel> model;
	};

	static EnginePrivate *getD(Engine *engine);
	static AsyncWatcherBackend *obtainAWB(Engine *engine);

	QScopedPointer<__private::SetupPrivate> setup;
	FirebaseAuthenticator *authenticator = nullptr;
	ICloudTransformer *transformer = nullptr;
	RemoteConnector *connector = nullptr;
	QHash<QString, DatabaseWatcher*> watchers;
	AsyncWatcherBackend *asyncWatcher = nullptr;

	QHash<QString, SettingsAdaptor*> settingsAdaptors;

	EngineStateMachine *engineMachine = nullptr;
	QPointer<EngineDataModel> engineModel;
	QHash<QString, TableInfo> tableMachines;
	QSet<QString> stopCache;

#ifndef QTDATASYNC_NO_NTP
	NtpSync *ntpSync = nullptr;
#endif

	DatabaseWatcher *getWatcher(QSqlDatabase &&database);
	DatabaseWatcher *findWatcher(const QString &name) const;
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

Q_DECLARE_METATYPE(QtDataSync::EnginePrivate::ErrorInfo);

#endif // QTDATASYNC_ENGINE_P_H
