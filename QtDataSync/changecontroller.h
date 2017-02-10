#ifndef CHANGECONTROLLER_H
#define CHANGECONTROLLER_H

#include <QJsonValue>
#include <QObject>
#include "datamerger.h"
#include "stateholder.h"

namespace QtDataSync {

class ChangeController : public QObject
{
	Q_OBJECT

public:
	enum Operation {
		Load,
		Save,
		Remove,
		MarkUnchanged
	};

	struct ChangeOperation {
		ObjectKey key;
		Operation operation;
		QJsonObject writeObject;
	};

	explicit ChangeController(DataMerger *merger, QObject *parent = nullptr);

	void initialize();
	void finalize();

public slots:
	void setInitialLocalStatus(const StateHolder::ChangeHash &changes);
	void updateLocalStatus(const ObjectKey &key, QtDataSync::StateHolder::ChangeState &state);
	void updateRemoteStatus(bool canUpdate, const StateHolder::ChangeHash &changes);

	void nextStage(bool success, const QJsonValue &result = QJsonValue::Undefined);

signals:
	void loadLocalStatus();

	void beginRemoteOperation(const ChangeOperation &operation);
	void beginLocalOperation(const ChangeOperation &operation);

private:
	enum ActionMode {
		DoNothing,
		DownloadRemote,
		DeleteLocal,
		UploadLocal,
		Merge,
		DeleteRemote,
		MarkAsUnchanged
	};

	enum ActionState {
		DoneState,
		DownloadState,
		SaveState,
		RemoteMarkState,
		RemoveLocalState,
		UploadState,
		LocalMarkState,
		LoadState,
		RemoveRemoteState
	};

	DataMerger *merger;

	bool localReady;
	bool remoteReady;

	StateHolder::ChangeHash localState;
	StateHolder::ChangeHash remoteState;

	ActionMode currentMode;
	ObjectKey currentKey;
	ActionState currentState;

	QJsonObject currentObject;

	void newChanges();

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

#endif // CHANGECONTROLLER_H
