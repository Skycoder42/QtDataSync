#ifndef QTDATASYNC_STORAGEENGINE_P_H
#define QTDATASYNC_STORAGEENGINE_P_H

#include "qtdatasync_global.h"
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
		Remove,
		Search
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
	void triggerResync();

Q_SIGNALS:
	void notifyChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void notifyResetted();

	void syncStateChanged(SyncController::SyncState syncState);
	void syncOperationsChanged(int remainingOperations);
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

	void performLocalReset(bool clearStore);

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
		bool isDeleteAction;

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
	void search(QFutureInterface<QVariant> futureInterface, QThread *targetThread, int dataMetaTypeId, QPair<int, QString> data);

	void tryMoveToThread(QVariant object, QThread *thread) const;
};

}

#endif // QTDATASYNC_STORAGEENGINE_P_H
