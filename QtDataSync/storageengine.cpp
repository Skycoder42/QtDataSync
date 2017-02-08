#include "storageengine.h"
#include <QThread>
using namespace QtDataSync;

StorageEngine::StorageEngine(QJsonSerializer *serializer) :
	QObject()
{
	serializer->setParent(this);
}

void StorageEngine::beginTask(QFutureInterface<QVariant> futureInterface, StorageEngine::TaskType taskType, int metaTypeId, const QString &key, const QVariant &value)
{

}

void StorageEngine::initialize()
{
}

void StorageEngine::finalize()
{
	QThread::sleep(2);
	thread()->quit();
}
