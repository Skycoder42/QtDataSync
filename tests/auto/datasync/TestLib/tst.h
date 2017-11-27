#ifndef TST_H
#define TST_H

#include <QtTest>

namespace Tst {

template <typename T>
bool compareUnordered(QList<T> actual, QList<T> expected, QByteArray aName, QByteArray eName, const char *file, int line) {
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
	foreach(auto a, actual) {
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

#endif // TST_H
