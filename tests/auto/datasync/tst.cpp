#include "tst.h"

void tst_init()
{
	QJsonSerializer::registerListConverters<TestData>();
}

void mockSetup(QtDataSync::Setup &setup)
{
	setup.setLocalStore(new MockLocalStore());
}

QList<TestData> generateData(int from, int to)
{
	QList<TestData> list;
	for(auto i = from; i < to; i++)
		list.append({i, QString::number(i)});
	return list;
}

DataSet generateDataJson(int from, int to)
{
	DataSet hash;
	for(auto i = from; i < to; i++) {
		QJsonObject data;
		data["id"] = i;
		data["text"] = QString::number(i);
		hash.insert({"TestData", QString::number(i)}, data);
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
