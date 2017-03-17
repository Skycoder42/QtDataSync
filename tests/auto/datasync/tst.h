#ifndef TST_H
#define TST_H

#include <QtDataSync>
#include <QtTest>

#include "testdata.h"
#include "mocklocalstore.h"

void tst_init();
void mockSetup(QtDataSync::Setup &setup);

typedef QHash<QtDataSync::ObjectKey, QJsonObject> DataSet;

QList<TestData> generateData(int from, int to);
DataSet generateDataJson(int from, int to);
QJsonArray dataListJson(const DataSet &data);

template <typename T>
bool qListCompare(QList<T> t1, QList<T> t2, const char *actual, const char *expected, const char *file, int line)
{
	if(!QTest::qCompare(t1.size(), t2.size(), actual, expected, file, line))
		return false;
	foreach(auto e, t1) {
		if(!QTest::qVerify(t2.contains(e), actual, expected, file, line))
			return false;
	}

	return true;
}

#define QLISTCOMPARE(actual, expected) \
	do {\
		if (!qListCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))\
			return;\
	} while (0)

#endif // TST_H
