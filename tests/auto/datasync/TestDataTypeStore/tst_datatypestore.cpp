#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <testobject.h>
using namespace QtDataSync;

class TestDataTypeStore : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testSimple();
	void testCachingGadget();
	void testCachingObject();

private:
	DataStore *dataStore;

	template<typename T>
	void testCaching(std::function<QList<T>(int,int)> generator,
					 std::function<bool(T,T)> equals = [](T a, T b){ return a == b; });
};

void TestDataTypeStore::initTestCase()
{
	try {
		Setup setup;
		TestLib::setup(setup);
		setup.create();

		dataStore = new DataStore(this);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestDataTypeStore::cleanupTestCase()
{
	dataStore->deleteLater();
	dataStore = nullptr;
	Setup::removeSetup(DefaultSetup);
}

void TestDataTypeStore::testSimple()
{
	dataStore->save(TestLib::generateData(3));

	DataTypeStore<TestData, int> store(dataStore, this);

	QSignalSpy changeSpy(&store, &DataTypeStoreBase::dataChanged);
	QSignalSpy resetSpy(&store, &DataTypeStoreBase::dataResetted);

	try {
		QCOMPARE(store.count(), 1);
		QCOMPAREUNORDERED(store.keys(), {3});

		for(auto i = 0; i < 3; i++) {
			auto data = TestLib::generateData(i);
			store.save(data);
			QCOMPARE(dataStore->load<TestData>(i), data);
			QCOMPARE(store.load(i), data);

			QCOMPARE(changeSpy.size(), 1);
			auto sig = changeSpy.takeFirst();
			QCOMPARE(sig[0].toInt(), i);
			QCOMPARE(sig[1].value<TestData>(), data);
		}

		QCOMPARE(store.count(), 4);
		QList<int> k = {0, 1, 2, 3};
		QCOMPAREUNORDERED(store.keys(), k);
		QCOMPAREUNORDERED(store.loadAll(), TestLib::generateData(0, 3));

		QVERIFY(store.remove(1));
		QCOMPARE(store.count(), 3);

		QCOMPARE(changeSpy.size(), 1);
		auto sig = changeSpy.takeFirst();
		QCOMPARE(sig[0].toInt(), 1);
		QVERIFY(!sig[1].isValid());

		store.clear();
		QCOMPARE(store.count(), 0);
		QCOMPARE(resetSpy.size(), 1);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestDataTypeStore::testCachingGadget()
{
	testCaching<TestData>([](int from, int to) {
		return TestLib::generateData(from, to);
	});
}

void TestDataTypeStore::testCachingObject()
{
	testCaching<TestObject*>([this](int from, int to) {
		QList<TestObject*> l;
		for(auto i = from; i <= to; i++) {
			auto obj = new TestObject(this);
			obj->id = i;
			obj->text = QString::number(i);
			l.append(obj);
		}
		return l;
	}, [](TestObject *a, TestObject *b) {
		if(a == b)
			return true;
		else if(!a || !b)
			return false;
		else
			return a->equals(b);
	});
}

template<typename T>
void TestDataTypeStore::testCaching(std::function<QList<T>(int,int)> generator, std::function<bool(T,T)> equals)
{
	dataStore->save(generator(3, 3).first());

	CachingDataTypeStore<T, int> store(dataStore, this);

	QSignalSpy changeSpy(&store, &DataTypeStoreBase::dataChanged);
	QSignalSpy resetSpy(&store, &DataTypeStoreBase::dataResetted);

	try {
		QCOMPARE(store.count(), 1);
		QCOMPAREUNORDERED(store.keys(), {3});
		QVERIFY(store.contains(3));

		for(auto i = 0; i < 3; i++) {
			QVERIFY(!store.contains(i));

			auto data = generator(i, i).first();
			store.save(data);
			QVERIFY(equals(dataStore->load<T>(i), data));
			QVERIFY(equals(store.load(i), data));
			QVERIFY(store.contains(i));

			QCOMPARE(changeSpy.size(), 1);
			auto sig = changeSpy.takeFirst();
			QCOMPARE(sig[0].toInt(), i);
			QVERIFY(equals(sig[1].value<T>(), data));
		}

		QCOMPARE(store.count(), 4);
		QList<int> k = {0, 1, 2, 3};
		QCOMPAREUNORDERED(store.keys(), k);
		QCOMPARE(store.loadAll().size(), 4);

		QVERIFY(store.remove(1));
		QCOMPARE(store.count(), 3);
		QVERIFY(!store.contains(1));

		QCOMPARE(changeSpy.size(), 1);
		auto sig = changeSpy.takeFirst();
		QCOMPARE(sig[0].toInt(), 1);
		QVERIFY(!sig[1].isValid());

		auto val = store.take(3);
		QCOMPARE(store.count(), 2);
		QVERIFY(!store.contains(3));
		QVERIFY(equals(val, generator(3, 3).first()));

		QCOMPARE(changeSpy.size(), 1);
		sig = changeSpy.takeFirst();
		QCOMPARE(sig[0].toInt(), 3);
		QVERIFY(!sig[1].isValid());

		store.clear();
		QCOMPARE(store.count(), 0);
		QCOMPARE(resetSpy.size(), 1);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

static void dataTypeStoreCompiletest_DO_NOT_CALL()
{
	DataTypeStore<TestData, int> t1;
	t1.count();
	t1.keys();
	t1.load(0);
	t1.loadAll();
	t1.save(TestData());
	t1.remove(5);
	t1.search(QStringLiteral("47"));
	t1.iterate([](TestData) {
		return false;
	});
	t1.clear();

	CachingDataTypeStore<TestData, int> t2;
	t2.count();
	t2.keys();
	t2.contains(42);
	t2.load(0);
	t2.loadAll();
	t2.save(TestData());
	t2.remove(5);
	t2.take(4);
	t2.clear();

	CachingDataTypeStore<TestObject*, int> t3;
	t2.count();
	t2.keys();
	t2.contains(42);
	t2.load(0);
	t2.loadAll();
	t2.save(TestData());
	t2.remove(5);
	t2.take(4);
	t2.clear();
}

QTEST_MAIN(TestDataTypeStore)

#include "tst_datatypestore.moc"
