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

	explicit ChangeController(DataMerger *merger, QObject *parent = nullptr);

	void initialize();
	void finalize();

public slots:
	void setInitialLocalStatus(const QtDataSync::StateHolder::ChangeHash &changes);
	void updateLocalStatus(const QtDataSync::StateHolder::ChangeKey &key, QtDataSync::StateHolder::ChangeState &state);
	void updateRemoteStatus(bool canUpdate, const QtDataSync::StateHolder::ChangeHash &changes);

	void nextStage(bool success, const QVariant &result);

signals:
	void loadLocalStatus();

	void beginRemoteOperation(const QtDataSync::StateHolder::ChangeKey &key, const QString &value, QtDataSync::ChangeController::Operation operation, const QJsonValue &writeObject);
	void beginLocalOperation(const QtDataSync::StateHolder::ChangeKey &key, const QString &value, QtDataSync::ChangeController::Operation operation, const QJsonValue &writeObject);

private:
	struct ChangeOperation {
		StateHolder::ChangeKey key;
		QString value;
		QJsonValue writeObject;

		Operation remoteOperation;
	};
	DataMerger *merger;

	bool localReady;
	bool remoteReady;

	StateHolder::ChangeHash localState;
	StateHolder::ChangeHash remoteState;

	void reloadChangeList();
};

}

#endif // CHANGECONTROLLER_H
