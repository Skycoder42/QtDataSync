#include "tst.h"
using namespace QtDataSync;

static QScopedPointer<QTemporaryDir> tDir;

void tst_init()
{
	QJsonSerializer::registerListConverters<TestData>();
	qRegisterMetaType<QtDataSync::SyncController::SyncState>("SyncState");
}

void mockSetup(QtDataSync::Setup &setup, bool autoRem)
{
	tDir.reset(new QTemporaryDir());
	tDir->setAutoRemove(autoRem);
	setup.setLocalStore(new MockLocalStore())
		 .setStateHolder(new MockStateHolder())
		 .setRemoteConnector(new MockRemoteConnector())
		 .setDataMerger(new MockDataMerger())
		 .setLocalDir(tDir->path());
}

TestData generateData(int index)
{
	return {index, QString::number(index)};
}

ObjectKey generateKey(int index)
{
	return {"TestData", QString::number(index)};
}

QList<TestData> generateData(int from, int to)
{
	QList<TestData> list;
	for(auto i = from; i < to; i++)
		list.append({i, QString::number(i)});
	return list;
}

QStringList generateDataKeys(int from, int to)
{
	QStringList list;
	for(auto i = from; i < to; i++)
		list.append(QString::number(i));
	return list;
}

DataSet generateDataJson(int from, int to)
{
	DataSet hash;
	for(auto i = from; i < to; i++)
		hash.insert(generateKey(i), generateDataJson(i));
	return hash;
}

QJsonObject generateDataJson(int index)
{
	QJsonObject data;
	data["id"] = index;
	data["text"] = QString::number(index);
	return data;
}

QJsonArray dataListJson(const DataSet &data)
{
	QJsonArray v;
	foreach(auto d, data)
		v.append(d);
	return v;
}

StateHolder::ChangeHash generateChangeHash(int from, int to, StateHolder::ChangeState state)
{
	StateHolder::ChangeHash hash;
	for(auto i = from; i < to; i++) {
		hash.insert(generateKey(i), state);
	}
	return hash;
}
