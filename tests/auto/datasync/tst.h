#ifndef TST_H
#define TST_H

#include <QtDataSync>
#include <QtTest>

#include "testdata.h"
#include "testobject.h"
#include "mocklocalstore.h"
#include "mockstateholder.h"
#include "mockremoteconnector.h"
#include "mockdatamerger.h"

void tst_init();
void mockSetup(QtDataSync::Setup &setup, bool autoRem = true);

typedef QHash<QtDataSync::ObjectKey, QJsonObject> DataSet;

QList<TestData> generateData(int from, int to);
TestData generateData(int index);
QtDataSync::ObjectKey generateKey(int index);
QStringList generateDataKeys(int from, int to);
DataSet generateDataJson(int from, int to);
QJsonObject generateDataJson(int index);
QJsonArray dataListJson(const DataSet &data);
QtDataSync::StateHolder::ChangeHash generateChangeHash(int from, int to, QtDataSync::StateHolder::ChangeState state);

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
