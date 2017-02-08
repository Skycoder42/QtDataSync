#include "exception.h"
#include "storageengine.h"
#include <QThread>
using namespace QtDataSync;

StorageEngine::StorageEngine(QJsonSerializer *serializer) :
	QObject(),
	serializer(serializer)
{
	serializer->setParent(this);
}

void StorageEngine::beginTask(QFutureInterface<QVariant> futureInterface, StorageEngine::TaskType taskType, int metaTypeId, const QString &key, const QVariant &value)
{
	try {
		switch (taskType) {
		case QtDataSync::StorageEngine::LoadAll:
			loadAll(futureInterface, metaTypeId);
			break;
		case QtDataSync::StorageEngine::Load:
			load(futureInterface, metaTypeId, key);
			break;
		case QtDataSync::StorageEngine::Save:
			save(futureInterface, metaTypeId, key, value);
			break;
		case QtDataSync::StorageEngine::Remove:
			remove(futureInterface, metaTypeId, key);
			break;
		case QtDataSync::StorageEngine::RemoveAll:
			removeAll(futureInterface, metaTypeId);
			break;
		default:
			break;
		}
	} catch(QException &e) {
		futureInterface.reportException(e);
		futureInterface.reportFinished();
	}
}

void StorageEngine::initialize()
{
}

void StorageEngine::finalize()
{
	thread()->quit();
}

void StorageEngine::loadAll(QFutureInterface<QVariant> futureInterface, int metaTypeId)
{
	try {
		auto list = serializer->deserialize(QJsonArray(), metaTypeId);
		qDebug() << list;

		futureInterface.reportResult(list);
		futureInterface.reportFinished();
	} catch(SerializerException &e) {
		throw Exception(e.qWhat());
	}
}

void StorageEngine::load(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key)
{
	try {
		auto value = serializer->deserialize(QJsonValue(), metaTypeId);
		qDebug() << value;

		futureInterface.reportResult(value);
		futureInterface.reportFinished();
	} catch(SerializerException &e) {
		throw Exception(e.qWhat());
	}
}

void StorageEngine::save(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, QVariant value)
{
	if(!value.convert(metaTypeId))
		throw Exception(QStringLiteral("Failed to convert value to %1").arg(QMetaType::typeName(metaTypeId)));

	try {
		auto json = serializer->serialize(value);
		qDebug() << json;

		futureInterface.reportFinished();
	} catch(SerializerException &e) {
		throw Exception(e.qWhat());
	}
}

void StorageEngine::remove(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key)
{
	futureInterface.reportFinished();
}

void StorageEngine::removeAll(QFutureInterface<QVariant> futureInterface, int metaTypeId)
{
	futureInterface.reportFinished();
}
