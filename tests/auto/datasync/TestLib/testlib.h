#ifndef TESTLIB_H
#define TESTLIB_H

#include <QtDataSync>
#include <QtTest>
#include "testdata.h"

#ifndef KEYSTORE_PATH
#define KEYSTORE_PATH ""
#endif

class TestLib
{
public:
	typedef QHash<QtDataSync::ObjectKey, QJsonObject> DataSet;
	static const QByteArray TypeName;

	static void init(const QByteArray &keystorePath = KEYSTORE_PATH);
	static QtDataSync::Setup &setup(QtDataSync::Setup &setup);

	static QtDataSync::ObjectKey generateKey(int index);
	static TestData generateData(int index);
	static QList<TestData> generateData(int from, int to);
	static QString generateDataKey(int index);
	static QStringList generateDataKeys(int from, int to);
	static QJsonObject generateDataJson(int index, const QString &specialText = {});
	static DataSet generateDataJson(int from, int to);
	static QJsonArray dataListJson(const DataSet &data);

	static QTemporaryDir tDir;
};

namespace Tst {

template <typename T>
bool compareUnordered(const QList<T> &actual, QList<T> expected, QByteArray aName, QByteArray eName, const char *file, int line) {
	if(actual.size() != expected.size()) {
		QTest::qFail(aName + " and " + eName +
					 " differ in size (" +
					 QByteArray::number(actual.size()) + " vs. " +
					 QByteArray::number(expected.size()) + ")",
					 file,
					 line);
		return false;
	}

	auto i = 0;
	for(auto a : actual) {
		if(!expected.removeOne(a)) {
			QTest::qFail("Data of " + aName +
						 " at index" + QByteArray::number(i) +
						 " is not contained in " + eName,
						 file,
						 line);
			return false;
		}
	}

	return true;
}

}

#define QCOMPAREUNORDERED(actual, expected) \
	do {\
		if (!Tst::compareUnordered(actual, expected, #actual, #expected, __FILE__, __LINE__))\
			return;\
	} while (false)

#endif // TESTLIB_H
