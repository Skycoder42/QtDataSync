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
	Q_ENUM(TaskType)

	explicit StorageEngine(QJsonSerializer *serializer);

public slots:
	void beginTask(QFutureInterface<QVariant> futureInterface, QtDataSync::StorageEngine::TaskType taskType, int metaTypeId, const QVariant &value = {});

private slots:
	void initialize();
	void finalize();

private:
	QJsonSerializer *serializer;

	void loadAll(QFutureInterface<QVariant> futureInterface, int metaTypeId);
	void load(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, const QString &value);
	void save(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, QVariant value);
	void remove(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, const QString &value);
	void removeAll(QFutureInterface<QVariant> futureInterface, int metaTypeId);
};

}

#endif // STORAGEENGINE_H
