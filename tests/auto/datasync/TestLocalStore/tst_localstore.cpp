#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QtConcurrent>
#include <testlib.h>
#include <QtDataSync/private/localstore_p.h>
#include <QtDataSync/private/defaults_p.h>
using namespace QtDataSync;

class TestLocalStore : public QObject
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
	void testUncached();
	void testRemove_data();
	void testRemove();
	void testClear();

	void testChangeSignals();
	void testAsync();

private:
	LocalStore *store;

	static void testAllImpl(LocalStore *store);
};

void TestLocalStore::initTestCase()
{
	try {
		TestLib::init();
		Setup setup;
		TestLib::setup(setup);
		setup.create();

		store = new LocalStore(DefaultsPrivate::obtainDefaults(DefaultSetup), this);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::cleanupTestCase()
{
	delete store;
	store = nullptr;
	Setup::removeSetup(DefaultSetup, true);
}

void TestLocalStore::testEmpty()
{
	try {
		QCOMPARE(store->count(TestLib::TypeName), 0ull);
		QVERIFY(store->keys(TestLib::TypeName).isEmpty());
		QVERIFY_EXCEPTION_THROWN(store->load(TestLib::generateKey(1)), NoDataException);
		QCOMPARE(store->remove(TestLib::generateKey(2)), false);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testSave_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<QJsonObject>("data");

	QTest::newRow("data0") << TestLib::generateKey(429)
						   << TestLib::generateDataJson(429);
	QTest::newRow("data1") << TestLib::generateKey(430)
						   << TestLib::generateDataJson(430);
	QTest::newRow("data2") << TestLib::generateKey(431)
						   << TestLib::generateDataJson(431);
	QTest::newRow("data3") << TestLib::generateKey(432)
						   << TestLib::generateDataJson(432);
}

void TestLocalStore::testSave()
{
	QFETCH(ObjectKey, key);
	QFETCH(QJsonObject, data);

	try {
		store->save(key, data);
		auto res = store->load(key);
		QCOMPARE(res, data);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testAll()
{
	testAllImpl(store);
}

void TestLocalStore::testFind()
{
	const QList<QJsonObject> objects {
		TestLib::generateDataJson(429),
		TestLib::generateDataJson(432)
	};

	try {
		QCOMPARE(store->find(TestLib::TypeName, QStringLiteral("*2*")), objects);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testUncached()
{
	LocalStore second(DefaultsPrivate::obtainDefaults(DefaultSetup));
	second.setCacheSize(0);
	testAllImpl(&second);
}

void TestLocalStore::testRemove_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<bool>("result");

	QTest::newRow("data0") << TestLib::generateKey(429)
						   << true;
	QTest::newRow("data1") << TestLib::generateKey(430)
						   << true;
	QTest::newRow("data2") << TestLib::generateKey(430)
						   << false;
}

void TestLocalStore::testRemove()
{
	QFETCH(ObjectKey, key);
	QFETCH(bool, result);

	try {
		QCOMPARE(store->remove(key), result);
		QVERIFY_EXCEPTION_THROWN(store->load(key), NoDataException);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testClear()
{
	try {
		//clear
		QCOMPARE(store->count(TestLib::TypeName), 2ull);
		store->clear(TestLib::TypeName);
		QCOMPARE(store->count(TestLib::TypeName), 0ull);

		//reset
		store->save(TestLib::generateKey(42), TestLib::generateDataJson(42));
		QCOMPARE(store->count(TestLib::TypeName), 1ull);
		store->reset(false);
		QCOMPARE(store->count(TestLib::TypeName), 0ull);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testChangeSignals()
{
	const auto key = TestLib::generateKey(77);
	auto data = TestLib::generateDataJson(77);

	LocalStore second(DefaultsPrivate::obtainDefaults(DefaultSetup));

	QSignalSpy store1Spy(store, &LocalStore::dataChanged);
	QSignalSpy store2Spy(&second, &LocalStore::dataChanged);

	try {
		store->save(key, data);
		QCOMPARE(store->load(key), data);
		QCOMPARE(second.load(key), data);

		QCOMPARE(store1Spy.size(), 1);
		auto sig = store1Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		QVERIFY(store2Spy.wait());
		QCOMPARE(store2Spy.size(), 1);
		sig = store2Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		data.insert(QStringLiteral("baum"), 42);
		second.save(key, data);
		QCOMPARE(second.load(key), data);
		QCOMPARE(store->load(key), data);

		QCOMPARE(store2Spy.size(), 1);
		sig = store2Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		QVERIFY(store1Spy.wait());
		QCOMPARE(store1Spy.size(), 1);
		sig = store1Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		QVERIFY(store->remove(key));
		QVERIFY_EXCEPTION_THROWN(store->load(key), NoDataException);
		QVERIFY_EXCEPTION_THROWN(second.load(key), NoDataException);

		QCOMPARE(store1Spy.size(), 1);
		sig = store1Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), true);

		QVERIFY(store2Spy.wait());
		QCOMPARE(store2Spy.size(), 1);
		sig = store2Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), true);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testAsync()
{
	auto cnt = 10 * QThread::idealThreadCount();

	QList<QFuture<void>> futures;
	for(auto i = 0; i < cnt; i++) {
		futures.append(QtConcurrent::run([&](){
			LocalStore lStore(DefaultsPrivate::obtainDefaults(DefaultSetup));//thread without eventloop!
			auto key = TestLib::generateKey(66);
			auto data = TestLib::generateDataJson(66);

			//try to provoke conflicts
			lStore.save(key, data);
			lStore.load(key);
			lStore.load(key);
			lStore.save(key, data);
			lStore.load(key);
			if(lStore.count(TestLib::TypeName) != 1)
				throw Exception(DefaultSetup, QStringLiteral("Expected count is not 1"));
		}));
	}

	try {
		foreach(auto f, futures)
			f.waitForFinished();
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testAllImpl(LocalStore *store)
{
	const quint64 count = 4;
	const QStringList keys {
		QStringLiteral("429"),
		QStringLiteral("430"),
		QStringLiteral("431"),
		QStringLiteral("432")
	};
	const QList<QJsonObject> objects = TestLib::generateDataJson(429, 432).values();

	try {
		QCOMPARE(store->count(TestLib::TypeName), count);
		QCOMPAREUNORDERED(store->keys(TestLib::TypeName), keys);
		QCOMPAREUNORDERED(store->loadAll(TestLib::TypeName), objects);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(TestLocalStore)

#include "tst_localstore.moc"
