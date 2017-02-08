#ifndef STORAGEENGINE_H
#define STORAGEENGINE_H

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
		LoadAll,
		Load,
		Save,
		Remove,
		RemoveAll
	};

	explicit StorageEngine(QJsonSerializer *serializer);

public slots:
	void beginTask(QFutureInterface<QVariant> futureInterface, StorageEngine::TaskType taskType, int metaTypeId, const QString &key = {}, const QVariant &value = {});

private slots:
	void initialize();
	void finalize();
};

}

#endif // STORAGEENGINE_H
