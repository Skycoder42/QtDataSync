#ifndef STORAGEENGINE_P_H
#define STORAGEENGINE_P_H

#include "qdatasync_global.h"
#include "changecontroller_p.h"
#include "datamerger.h"
#include "defaults.h"
#include "localstore.h"
#include "remoteconnector.h"
#include "stateholder.h"

#include <QtCore/QDir>
#include <QtCore/QFuture>
#include <QtCore/QObject>

#include <QtJsonSerializer/QJsonSerializer>

namespace QtDataSync {

class Q_DATASYNC_EXPORT StorageEngine : public QObject
{
	Q_OBJECT
	friend class Setup;

	Q_PROPERTY(SyncController::SyncState syncState READ syncState NOTIFY syncStateChanged)
	Q_PROPERTY(QString authenticationError READ authenticationError NOTIFY authenticationErrorChanged)

public:
	enum TaskType {
		Count,
		Keys,
		LoadAll,
		Load,
		Save,
		Remove
	};
	Q_ENUM(TaskType)

	explicit StorageEngine(Defaults *defaults,
						   QJsonSerializer *serializer,
						   LocalStore *localStore,
						   StateHolder *stateHolder,
						   RemoteConnector *remoteConnector,
						   DataMerger *dataMerger);

	SyncController::SyncState syncState() const;
	QString authenticationError() const;

public Q_SLOTS:
	void beginTask(QFutureInterface<QVariant> futureInterface,
				   QThread *targetThread,
				   QtDataSync::StorageEngine::TaskType taskType,
				   int metaTypeId,
				   const QVariant &value = {});
	void triggerSync();

Q_SIGNALS:
	void notifyChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void syncStateChanged(SyncController::SyncState syncState);
	void authenticationErrorChanged(const QString &authenticationError);

private Q_SLOTS:
	void initialize();
	void finalize();

	void requestCompleted(quint64 id, const QJsonValue &result);
	void requestFailed(quint64 id, const QString &errorString);
	void operationDone(const QJsonValue &result);
	void operationFailed(const QString &errorString);
	void authError(const QString &reason);
	void clearAuthError();

	void loadLocalStatus();
	void updateSyncState(SyncController::SyncState state);
	void beginRemoteOperation(const ChangeController::ChangeOperation &operation);
	void beginLocalOperation(const ChangeController::ChangeOperation &operation);

private:
	struct Q_DATASYNC_EXPORT RequestInfo {
		//change controller
		bool isChangeControllerRequest;

		//store requests
		QFutureInterface<QVariant> futureInterface;
		QThread *targetThread;
		int convertMetaTypeId;

		//change notifying
		ObjectKey notifyKey;
		bool notifyChanged;

		//changing operations
		bool changeAction;
		ObjectKey changeKey;
		StateHolder::ChangeState changeState;

		RequestInfo(bool isChangeControllerRequest = false);
		RequestInfo(QFutureInterface<QVariant> futureInterface,
					QThread *targetThread,
					int convertMetaTypeId = QMetaType::UnknownType);
	};

	Defaults *defaults;
	QJsonSerializer *serializer;
	LocalStore *localStore;
	StateHolder *stateHolder;
	RemoteConnector *remoteConnector;
	ChangeController *changeController;

	QHash<quint64, RequestInfo> requestCache;
	quint64 requestCounter;

	SyncController::SyncState currentSyncState;
	QString currentAuthError;

	void count(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int metaTypeId);
	void keys(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int metaTypeId);
	void loadAll(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int dataMetaTypeId, int listMetaTypeId);
	void load(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int metaTypeId, const QByteArray &keyProperty, const QString &value);
	void save(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int metaTypeId, const QByteArray &keyProperty, QVariant value);
	void remove(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int metaTypeId, const QByteArray &keyProperty, const QString &value);

	void tryMoveToThread(QVariant object, QThread *thread) const;
};

}

#endif // STORAGEENGINE_P_H
