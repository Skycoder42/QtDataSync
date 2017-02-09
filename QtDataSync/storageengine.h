#ifndef STORAGEENGINE_H
#define STORAGEENGINE_H

#include "datamerger.h"
#include "localstore.h"
#include "remoteconnector.h"
#include "stateholder.h"

#include <QFuture>
#include <QJsonSerializer>
#include <QObject>

namespace QtDataSync {

class StorageEngine : public QObject
{
	Q_OBJECT
	friend class Setup;

public:
	enum TaskType {
		Count,
		LoadAll,
		Load,
		Save,
		Remove,
		RemoveAll
	};
	Q_ENUM(TaskType)

	explicit StorageEngine(QJsonSerializer *serializer,
						   LocalStore *localStore,
						   StateHolder *stateHolder,
						   RemoteConnector *remoteConnector,
						   DataMerger *dataMerger);

public slots:
	void beginTask(QFutureInterface<QVariant> futureInterface, QtDataSync::StorageEngine::TaskType taskType, int metaTypeId, const QVariant &value = {});

private slots:
	void initialize();
	void finalize();

	void requestCompleted(quint64 id, const QJsonValue &result);
	void requestFailed(quint64 id, const QString &errorString);

private:
	struct RequestInfo {
		QFutureInterface<QVariant> futureInterface;
		int metaTypeId;

		bool changeAction;
		QString changeKey;
		StateHolder::ChangeState changeState;

		RequestInfo(QFutureInterface<QVariant> futureInterface = {},
					int metaTypeId = QMetaType::UnknownType);
	};

	QJsonSerializer *serializer;
	LocalStore *localStore;
	StateHolder *stateHolder;
	RemoteConnector *remoteConnector;
	DataMerger *dataMerger;

	QHash<quint64, RequestInfo> requestCache;
	quint64 requestCounter;

	void count(QFutureInterface<QVariant> futureInterface, int metaTypeId);
	void loadAll(QFutureInterface<QVariant> futureInterface, int dataMetaTypeId, int listMetaTypeId);
	void load(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, const QString &value);
	void save(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, QVariant value);
	void remove(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, const QString &value);
	void removeAll(QFutureInterface<QVariant> futureInterface, int metaTypeId);
};

}

#endif // STORAGEENGINE_H
