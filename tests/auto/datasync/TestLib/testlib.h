#ifndef TESTLIB_H
#define TESTLIB_H

#include <QtDataSync>
#include "testdata.h"
#include "tst.h"

class TestLib
{
public:
	typedef QHash<QtDataSync::ObjectKey, QJsonObject> DataSet;
	static const QByteArray TypeName;

	static void init();
	static QtDataSync::Setup &setup(QtDataSync::Setup &setup);

	static QtDataSync::ObjectKey generateKey(int index);
	static TestData generateData(int index);
	static QList<TestData> generateData(int from, int to);
	static QStringList generateDataKeys(int from, int to);
	static QJsonObject generateDataJson(int index);
	static DataSet generateDataJson(int from, int to);
	static QJsonArray dataListJson(const DataSet &data);

	static QTemporaryDir tDir;
};

#endif // TESTLIB_H
