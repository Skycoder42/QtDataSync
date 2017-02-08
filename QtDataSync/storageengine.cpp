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
	QThread::sleep(2);
	futureInterface.reportResult(QVariant());
	futureInterface.reportFinished();
}

void StorageEngine::initialize()
{
}

void StorageEngine::finalize()
{
	thread()->quit();
}
