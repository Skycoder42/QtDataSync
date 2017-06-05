#ifndef QTDATASYNC_CHANGECONTROLLER_P_H
#define QTDATASYNC_CHANGECONTROLLER_P_H

#include "qtdatasync_global.h"
#include "datamerger.h"
#include "defaults.h"
#include "stateholder.h"
#include "synccontroller.h"
#include "remoteconnector.h"

#include <QtCore/QJsonValue>
#include <QtCore/QObject>

namespace QtDataSync {

class Q_DATASYNC_EXPORT ChangeController : public QObject
{
	Q_OBJECT

public:
	enum Operation {
		Load,
		Save,
		Remove,
		MarkUnchanged
	};
	Q_ENUM(Operation)

	enum ActionMode {
		DoNothing,
		DownloadRemote,
		DeleteLocal,
		UploadLocal,
		Merge,
		DeleteRemote,
		MarkAsUnchanged
	};
	Q_ENUM(ActionMode)

	enum ActionState {
		DoneState,
		CancelState,
		DownloadState,
		SaveState,
		RemoteMarkState,
		RemoveLocalState,
		UploadState,
		LocalMarkState,
		LoadState,
		RemoveRemoteState
	};
	Q_ENUM(ActionState)

	struct Q_DATASYNC_EXPORT ChangeOperation {
		ObjectKey key;
		Operation operation;
		QJsonObject writeObject;
	};

	explicit ChangeController(DataMerger *merger, QObject *parent = nullptr);

	void initialize(Defaults *defaults);
	void finalize();

public Q_SLOTS:
	void setInitialLocalStatus(const StateHolder::ChangeHash &changes, bool triggerSync);
	void updateLocalStatus(const ObjectKey &key, QtDataSync::StateHolder::ChangeState &state);

	void setRemoteStatus(RemoteConnector::RemoteState state, const StateHolder::ChangeHash &changes);
	void updateRemoteStatus(const ObjectKey &key, StateHolder::ChangeState state);

	void nextStage(bool success, const QJsonValue &result = QJsonValue::Undefined);

Q_SIGNALS:
	void loadLocalStatus();
	void updateSyncState(SyncController::SyncState state);
	void updateSyncProgress(int remainingOperations);

	void beginRemoteOperation(const ChangeOperation &operation);
	void beginLocalOperation(const ChangeOperation &operation);

private:
	Defaults *defaults;
	DataMerger *merger;

	bool localReady;
	bool remoteReady;

	StateHolder::ChangeHash localState;
	StateHolder::ChangeHash remoteState;
	QSet<ObjectKey> failedKeys;

	ActionMode currentMode;
	ObjectKey currentKey;
	ActionState currentState;
	QJsonObject currentObject;

	void newChanges();
	void updateProgress();

	void generateNextAction();
	void actionDownloadRemote(const QJsonValue &result);
	void actionDeleteLocal();
	void actionUploadLocal(const QJsonValue &result);
	void actionMerge(const QJsonValue &result);
	void actionDeleteRemote();
	void actionMarkAsUnchanged();
};

}

Q_DECLARE_METATYPE(QtDataSync::ChangeController::ChangeOperation)

#endif // QTDATASYNC_CHANGECONTROLLER_P_H
