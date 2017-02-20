#ifndef STORAGEENGINE_H
#define STORAGEENGINE_H

#include "changecontroller.h"
#include "datamerger.h"
#include "defaults.h"
#include "localstore.h"
#include "remoteconnector.h"
#include "stateholder.h"

#include <QDir>
#include <QFuture>
#include <QJsonSerializer>
#include <QObject>

namespace QtDataSync {

class StorageEngine : public QObject
{
	Q_OBJECT
	friend class Setup;

	Q_PROPERTY(SyncController::SyncState syncState READ syncState NOTIFY syncStateChanged)

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

public slots:
	void beginTask(QFutureInterface<QVariant> futureInterface, QtDataSync::StorageEngine::TaskType taskType, int metaTypeId, const QVariant &value = {});
	void triggerSync();

signals:
	void notifyChanged(int metaTypeId, const QString &key, bool wasDeleted);
	void syncStateChanged(SyncController::SyncState syncState);

private slots:
	void initialize();
	void finalize();

	void requestCompleted(quint64 id, const QJsonValue &result);
	void requestFailed(quint64 id, const QString &errorString);
	void operationDone(const QJsonValue &result);
	void operationFailed(const QString &errorString);

	void loadLocalStatus();
	void updateSyncState(SyncController::SyncState state);
	void beginRemoteOperation(const ChangeController::ChangeOperation &operation);
	void beginLocalOperation(const ChangeController::ChangeOperation &operation);

private:
	struct RequestInfo {
		//change controller
		bool isChangeControllerRequest;

		//store requests
		QFutureInterface<QVariant> futureInterface;
		int convertMetaTypeId;

		//change notifying
		ObjectKey notifyKey;
		bool notifyChanged;

		//changing operations
		bool changeAction;
		ObjectKey changeKey;
		StateHolder::ChangeState changeState;

		RequestInfo(bool isChangeControllerRequest = false);
		RequestInfo(QFutureInterface<QVariant> futureInterface, int convertMetaTypeId = QMetaType::UnknownType);
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

	void count(QFutureInterface<QVariant> futureInterface, int metaTypeId);
	void keys(QFutureInterface<QVariant> futureInterface, int metaTypeId);
	void loadAll(QFutureInterface<QVariant> futureInterface, int dataMetaTypeId, int listMetaTypeId);
	void load(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QByteArray &keyProperty, const QString &value);
	void save(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QByteArray &keyProperty, QVariant value);
	void remove(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QByteArray &keyProperty, const QString &value);
};

}

#endif // STORAGEENGINE_H
