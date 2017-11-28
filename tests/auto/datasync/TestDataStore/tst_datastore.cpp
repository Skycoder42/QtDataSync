#include "testobject.h"

#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
using namespace QtDataSync;

class TestDataStore : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testEmpty();
	void testSave_data();
	void testSave();
	void testAll();
	void testFind();
	void testRemove_data();
	void testRemove();
	void testClear();

	void testLoadInto();

	void testChangeSignals();

private:
	DataStore *store;
};

void TestDataStore::initTestCase()
{
	try {
		Setup setup;
		TestLib::setup(setup);
		setup.create();

		store = new DataStore(this);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestDataStore::cleanupTestCase()
{
	store->deleteLater();
	store = nullptr;
	Setup::removeSetup(DefaultSetup);
}

void TestDataStore::testEmpty()
{
	try {
		QCOMPARE(store->count<TestData>(), 0ull);
		QVERIFY(store->keys<TestData>().isEmpty());
		QVERIFY_EXCEPTION_THROWN(store->load<TestData>(77), NoDataException);
		QCOMPARE(store->remove<TestData>(77), false);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestDataStore::testSave_data()
{
	QTest::addColumn<TestData>("data");

	QTest::newRow("data0") << TestLib::generateData(429);
	QTest::newRow("data1") << TestLib::generateData(430);
	QTest::newRow("data2") << TestLib::generateData(431);
	QTest::newRow("data3") << TestLib::generateData(432);
}

void TestDataStore::testSave()
{
	QFETCH(TestData, data);

	try {
		store->save(data);
		auto res = store->load<TestData>(data.id);
		QCOMPARE(res, data);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestDataStore::testAll()
{
	const quint64 count = 4;
	const QList<int> keys { 429, 430, 431, 432 };
	const QList<TestData> objects = TestLib::generateData(429, 432);

	try {
		QCOMPARE(store->count<TestData>(), count);
		auto k = store->keys<TestData, int>();
		QCOMPAREUNORDERED(k, keys);
		QCOMPAREUNORDERED(store->loadAll<TestData>(), objects);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestDataStore::testFind()
{
	const QList<TestData> objects {
		TestLib::generateData(429),
		TestLib::generateData(432)
	};

	try {
		QCOMPARE(store->search<TestData>(QStringLiteral("*2*")), objects);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestDataStore::testRemove_data()
{
	QTest::addColumn<int>("key");

	QTest::newRow("data0") << 429;
	QTest::newRow("data1") << 430;
}

void TestDataStore::testRemove()
{
	QFETCH(int, key);

	try {
		store->remove<TestData>(key);
		QVERIFY_EXCEPTION_THROWN(store->load<TestData>(key), NoDataException);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestDataStore::testClear()
{
	try {
		//clear
		QCOMPARE(store->count<TestData>(), 2ull);
		store->clear<TestData>();
		QCOMPARE(store->count<TestData>(), 0ull);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestDataStore::testLoadInto()
{
	auto dataObj = new TestObject(this);
	dataObj->id = 10;
	dataObj->text = QStringLiteral("baum");

	try {
		store->save(dataObj);

		auto d2 = store->load<TestObject*>(dataObj->id);
		QVERIFY(d2);
		QCOMPARE(d2->id, dataObj->id);
		QCOMPARE(d2->text, dataObj->text);

		dataObj->text = QStringLiteral("something new");
		store->save(dataObj);

		QSignalSpy spy(d2, &TestObject::textChanged);
		store->update(d2);

		QCOMPARE(spy.size(), 1);
		auto f = spy.takeFirst();
		QCOMPARE(f[0].toString(), dataObj->text);

		QCOMPARE(d2->id, dataObj->id);
		QCOMPARE(d2->text, dataObj->text);

		d2->deleteLater();
	} catch(QException &e) {
		QFAIL(e.what());
	}

	dataObj->deleteLater();
}

void TestDataStore::testChangeSignals()
{
	const auto key = 77;
	auto data = TestLib::generateData(77);

	auto second = new DataStore(this);

	QSignalSpy store1Spy(store, &DataStore::dataChanged);
	QSignalSpy store2Spy(second, &DataStore::dataChanged);

	try {
		store->save(data);
		QCOMPARE(store->load<TestData>(key), data);
		QCOMPARE(second->load<TestData>(key), data);

		QCOMPARE(store1Spy.size(), 1);
		auto sig = store1Spy.takeFirst();
		QCOMPARE(sig[0].toInt(), qMetaTypeId<TestData>());
		QCOMPARE(sig[1].toInt(), key);
		QCOMPARE(sig[2].toBool(), false);

		QVERIFY(store2Spy.wait());
		QCOMPARE(store2Spy.size(), 1);
		sig = store2Spy.takeFirst();
		QCOMPARE(sig[0].toInt(), qMetaTypeId<TestData>());
		QCOMPARE(sig[1].toInt(), key);
		QCOMPARE(sig[2].toBool(), false);

		data.text = QStringLiteral("Some other text");
		second->save(data);
		QCOMPARE(second->load<TestData>(key), data);
		QCOMPARE(store->load<TestData>(key), data);

		QCOMPARE(store2Spy.size(), 1);
		sig = store2Spy.takeFirst();
		QCOMPARE(sig[0].toInt(), qMetaTypeId<TestData>());
		QCOMPARE(sig[1].toInt(), key);
		QCOMPARE(sig[2].toBool(), false);

		QVERIFY(store1Spy.wait());
		QCOMPARE(store1Spy.size(), 1);
		sig = store1Spy.takeFirst();
		QCOMPARE(sig[0].toInt(), qMetaTypeId<TestData>());
		QCOMPARE(sig[1].toInt(), key);
		QCOMPARE(sig[2].toBool(), false);

		store->remove<TestData>(key);
		QVERIFY_EXCEPTION_THROWN(store->load<TestData>(key), NoDataException);
		QVERIFY_EXCEPTION_THROWN(second->load<TestData>(key), NoDataException);

		QCOMPARE(store1Spy.size(), 1);
		sig = store1Spy.takeFirst();
		QCOMPARE(sig[0].toInt(), qMetaTypeId<TestData>());
		QCOMPARE(sig[1].toInt(), key);
		QCOMPARE(sig[2].toBool(), true);

		QVERIFY(store2Spy.wait());
		QCOMPARE(store2Spy.size(), 1);
		sig = store2Spy.takeFirst();
		QCOMPARE(sig[0].toInt(), qMetaTypeId<TestData>());
		QCOMPARE(sig[1].toInt(), key);
		QCOMPARE(sig[2].toBool(), true);
	} catch(QException &e) {
		QFAIL(e.what());
	}

	second->deleteLater();
}

QTEST_MAIN(TestDataStore)

#include "tst_datastore.moc"
