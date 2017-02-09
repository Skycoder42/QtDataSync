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
	DataMerger *merger;

	bool localReady;
	bool remoteReady;

	StateHolder::ChangeHash localState;
	StateHolder::ChangeHash remoteState;

	void reloadChangeList();
};

}

#endif // CHANGECONTROLLER_H
