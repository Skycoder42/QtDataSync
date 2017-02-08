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

void StorageEngine::beginTask(QFutureInterface<QVariant> futureInterface, StorageEngine::TaskType taskType, int metaTypeId, const QVariant &value)
{
	try {
		auto flags = QMetaType::typeFlags(metaTypeId);
		if(!flags.testFlag(QMetaType::PointerToQObject) &&
		   !flags.testFlag(QMetaType::IsGadget))
			throw Exception("You can only store QObjects or Q_GADGETs with QtDataSync!");

		auto metaObject = QMetaType::metaObjectForType(metaTypeId);
		auto userProp = metaObject->userProperty();

		switch (taskType) {
		case QtDataSync::StorageEngine::LoadAll:
			loadAll(futureInterface, metaTypeId);
			break;
		case QtDataSync::StorageEngine::Load:
			load(futureInterface, metaTypeId, userProp.name(), value.toString());
			break;
		case QtDataSync::StorageEngine::Save:
			save(futureInterface, metaTypeId, userProp.name(), value);
			break;
		case QtDataSync::StorageEngine::Remove:
			remove(futureInterface, metaTypeId, userProp.name(), value.toString());
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

void StorageEngine::load(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, const QString &value)
{
	try {
		auto obj = serializer->deserialize(QJsonValue(), metaTypeId);
		qDebug() << "Load for"
				 << key
				 << "with value"
				 << value
				 << "loaded:"
				 << obj;

		futureInterface.reportResult(obj);
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
		qDebug() << "Save for"
				 << key
				 << "of"
				 << json;

		futureInterface.reportFinished();
	} catch(SerializerException &e) {
		throw Exception(e.qWhat());
	}
}

void StorageEngine::remove(QFutureInterface<QVariant> futureInterface, int metaTypeId, const QString &key, const QString &value)
{
	futureInterface.reportFinished();
}

void StorageEngine::removeAll(QFutureInterface<QVariant> futureInterface, int metaTypeId)
{
	futureInterface.reportFinished();
}
