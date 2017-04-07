#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include "tst.h"
using namespace QtDataSync;

class CachingDataStoreTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testConstructionLoad();
	void testLoad();
	void testSave();
	void testDelete();

private:
	AsyncDataStore *async;
	CachingDataStore<TestData, int> *caching;
};

void CachingDataStoreTest::initTestCase()
{
#ifdef Q_OS_LINUX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	Setup setup;
	mockSetup(setup);
	static_cast<MockLocalStore*>(setup.localStore())->enabled = true;
	setup.create();

	async = new AsyncDataStore(this);
}

void CachingDataStoreTest::cleanupTestCase()
{
	delete caching;
	delete async;
	Setup::removeSetup(Setup::DefaultSetup);
}

void CachingDataStoreTest::testConstructionLoad()
{
	caching = new CachingDataStore<TestData, int>(this);

	QSignalSpy loadSpy(caching, &CachingDataStoreBase::storeLoaded);
	QVERIFY(loadSpy.wait());

	QCOMPARE(caching->count(), 0);
	QVERIFY(caching->keys().isEmpty());
	QVERIFY(caching->loadAll().isEmpty());
}

void CachingDataStoreTest::testLoad()
{
	QVERIFY(caching);

	try {
		QSignalSpy changedSpy(caching, &CachingDataStoreBase::dataChanged);

		//add
		QCOMPARE(caching->load(42), TestData());//not there
		QVERIFY(!caching->contains(42));
		async->save<TestData>(generateData(42));
		QCOMPARE(caching->load(42), TestData());//still not there

		QVERIFY(changedSpy.wait());
		QCOMPARE(changedSpy.size(), 1);
		QCOMPARE(changedSpy[0][0].toString(), QStringLiteral("42"));
		QCOMPARE(changedSpy[0][1].value<TestData>(), generateData(42));

		QCOMPARE(caching->load(42), generateData(42));
		QCOMPARE(caching->count(), 1);
		QCOMPARE(caching->keys(), QList<int>({42}));
		QVERIFY(caching->contains(42));

		//delete
		async->remove<TestData>(42);
		QVERIFY(changedSpy.wait());
		QCOMPARE(changedSpy.size(), 2);
		QCOMPARE(changedSpy[1][0].toString(), QStringLiteral("42"));
		QVERIFY(!changedSpy[1][1].isValid());

		QCOMPARE(caching->load(42), TestData());
		QCOMPARE(caching->count(), 0);
		QCOMPARE(caching->keys(), QList<int>());
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void CachingDataStoreTest::testSave()
{
	QVERIFY(caching);

	try {
		QSignalSpy changedSpy(caching, &CachingDataStoreBase::dataChanged);

		QCOMPARE(caching->load(42), TestData());
		caching->save(generateData(42));
		QCOMPARE(caching->load(42), generateData(42));
		QCOMPARE(caching->count(), 1);
		QCOMPARE(caching->keys(), QList<int>({42}));

		QVERIFY(changedSpy.wait());
		QCOMPARE(changedSpy.size(), 1);
		QCOMPARE(changedSpy[0][0].toString(), QStringLiteral("42"));
		QCOMPARE(changedSpy[0][1].value<TestData>(), generateData(42));

		QCOMPARE(async->load<TestData>(42).result(), generateData(42));
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void CachingDataStoreTest::testDelete()
{
	QVERIFY(caching);

	try {
		QSignalSpy changedSpy(caching, &CachingDataStoreBase::dataChanged);

		QCOMPARE(caching->load(42), generateData(42));
		caching->remove(42);
		QCOMPARE(caching->load(42), TestData());
		QCOMPARE(caching->count(), 0);
		QCOMPARE(caching->keys(), QList<int>());

		QVERIFY(changedSpy.wait());
		QCOMPARE(changedSpy.size(), 1);
		QCOMPARE(changedSpy[0][0].toString(), QStringLiteral("42"));
		QVERIFY(!changedSpy[0][1].isValid());

		QVERIFY_EXCEPTION_THROWN(async->load<TestData>(42).result(), DataSyncException);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(CachingDataStoreTest)

#include "tst_cachingdatastore.moc"
