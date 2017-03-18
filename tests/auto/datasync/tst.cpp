#include "tst.h"
using namespace QtDataSync;

void tst_init()
{
	QJsonSerializer::registerListConverters<TestData>();
	qRegisterMetaType<QtDataSync::SyncController::SyncState>("SyncState");
}

void mockSetup(QtDataSync::Setup &setup)
{
	setup.setLocalStore(new MockLocalStore())
		 .setStateHolder(new MockStateHolder())
		 .setRemoteConnector(new MockRemoteConnector())
		 .setDataMerger(new MockDataMerger());
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
	for(auto i = from; i < to; i++) {
		QJsonObject data;
		data["id"] = i;
		data["text"] = QString::number(i);
		hash.insert(generateKey(i), data);
	}
	return hash;
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
